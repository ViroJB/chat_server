#include "server.h"

Server::Server() : m_is_running(false) {}

Server::~Server() { Stop(); }

bool Server::InitializeWSA() {
    fmt::print("Initializing WSA...\n");
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        fmt::print("WSAStartup failed: {}\n", result);
        return false;
    }
    return true;
}

bool Server::Start(int port) {
    fmt::print("Starting server on port {}...\n", port);
    if (!InitializeWSA()) {
        return false;
    }

    m_server_socket = CreateServerSocket();
    if (m_server_socket == INVALID_SOCKET) {
        return false;
    }

    m_server_address.sin_family = AF_INET;
    m_server_address.sin_addr.s_addr = INADDR_ANY;
    m_server_address.sin_port = htons(port);

    if (!BindServerSocket(m_server_socket, m_server_address)) {
        closesocket(m_server_socket);
        WSACleanup();
        return false;
    }

    if (listen(m_server_socket, SOMAXCONN) == SOCKET_ERROR) {
        fmt::print("listen failed: {}\n", WSAGetLastError());
        closesocket(m_server_socket);
        WSACleanup();
        return false;
    }

    StartListening();
    return true;
}

SOCKET Server::CreateServerSocket() {
    fmt::print("Creating server socket...\n");
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        fmt::print("socket failed: {}\n", WSAGetLastError());
        WSACleanup();
        return INVALID_SOCKET;
    }
    return server_socket;
}

bool Server::BindServerSocket(SOCKET server_socket, sockaddr_in server_address) {
    fmt::print("Binding server socket...\n");
    int result = bind(server_socket, (sockaddr*)&server_address, sizeof(server_address));
    if (result == SOCKET_ERROR) {
        fmt::print("bind failed: {}\n", WSAGetLastError());
        return false;
    }
    return true;
}

void Server::StartListening() {
    fmt::print("Server is now listening for connections...\n");
    m_is_running = true;

    std::thread(&Server::AcceptConnections, this).detach();
    std::thread(&Server::HandleClientMessages, this).detach();
}

void Server::HandleClientMessages() {
    fmt::print("Starting to handle client messages...\n");
    while (m_is_running) {
        fd_set socket_set;
        SOCKET highest_socket;
        CheckSocketsActivity(socket_set, highest_socket);

        fmt::print("Waiting for socket activity...\n");
        int socket_activity = select(highest_socket + 1, &socket_set, NULL, NULL, NULL);
        if (socket_activity == SOCKET_ERROR) {
            fmt::print("select failed: {}\n", WSAGetLastError());
            continue;
        }

        fmt::print("Processing client messages...\n");
        ProcessClientMessages(socket_set);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Server::AcceptConnections() {
    fmt::print("Starting to accept connections...\n");
    while (m_is_running) {
        fd_set socket_set;
        SOCKET highest_socket;
        CheckSocketsActivity(socket_set, highest_socket);

        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // fmt::print("Waiting for new connections...\n");
        int socket_activity = select(highest_socket + 1, &socket_set, NULL, NULL, &timeout);
        if (socket_activity == SOCKET_ERROR) {
            fmt::print("select failed: {}\n", WSAGetLastError());
            continue;
        }

        if (socket_activity > 0 && FD_ISSET(m_server_socket, &socket_set)) {
            fmt::print("Socket activity detected.\n");
            ProcessIncomingConnection(socket_set);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Server::CheckSocketsActivity(fd_set& socket_set, SOCKET& highest_socket) {
    // fmt::print("Checking socket activity...\n");
    FD_ZERO(&socket_set);
    FD_SET(m_server_socket, &socket_set);
    highest_socket = m_server_socket;

    for (SOCKET client_socket : m_client_sockets) {
        FD_SET(client_socket, &socket_set);
        if (client_socket > highest_socket) {
            highest_socket = client_socket;
        }
    }
}

void Server::ProcessIncomingConnection(fd_set& socket_set) {
    fmt::print("Processing incoming connection...\n");
    if (FD_ISSET(m_server_socket, &socket_set)) {
        SOCKET client_socket = accept(m_server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            fmt::print("accept failed: {}\n", WSAGetLastError());
            return;
        }

        fmt::print("New connection accepted.\n");
        m_client_mutex.lock();
        m_client_sockets.push_back(client_socket);
        m_client_mutex.unlock();
    }
}

void Server::ProcessClientMessages(fd_set& socket_set) {
    fmt::print("Processing client messages...\n");
    std::vector<SOCKET> disconnected_sockets;
    m_client_mutex.lock();
    for (SOCKET client_socket : m_client_sockets) {
        if (FD_ISSET(client_socket, &socket_set)) {
            std::string message = ReceiveMessageFromClient(client_socket);
            if (message.empty()) {
                disconnected_sockets.push_back(client_socket);
                fmt::print("Client disconnected.\n");
                continue;
            }

            fmt::print("Message received: {}\n", message);
            BroadcastMessage(message, client_socket);
        }
    }

    // remove disconnected clients
    for (SOCKET client_socket : disconnected_sockets) {
        fmt::print("Removing disconnected client.\n");
        closesocket(client_socket);
        m_client_sockets.remove(client_socket);
    }
    m_client_mutex.unlock();
}

std::string Server::ReceiveMessageFromClient(SOCKET client_socket) {
    fmt::print("Receiving message...\n");
    char buffer[1024];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received == SOCKET_ERROR) {
        fmt::print("recv failed: {}\n", WSAGetLastError());
        return "";
    }

    buffer[bytes_received] = '\0';
    std::string message(buffer);
    fmt::print("Message received from client: {}\n", message);
    return message;
}

void Server::BroadcastMessage(const std::string& message, SOCKET sender_socket) {
    fmt::print("Broadcasting message: {}\n", message);

    for (SOCKET client_socket : m_client_sockets) {
        fmt::print("Sending message to client...\n");
        // if (client_socket != sender_socket) {
            fmt::print("Sending message to client...\n");
            int send_result = send(client_socket, message.c_str(), message.size(), 0);
            if (send_result == SOCKET_ERROR) {
                fmt::print("send failed: {}\n", WSAGetLastError());
            }
        // }
    }
}

bool Server::Stop() {
    fmt::print("Stopping server...\n");
    if (!m_is_running) {
        return false;
    }

    m_is_running = false;
    StopListening();
    Cleanup();
    return true;
}

void Server::StopListening() {
    fmt::print("Stopping listening...\n");
    m_is_running = false;
}

void Server::Cleanup() {
    fmt::print("Cleaning up...\n");
    closesocket(m_server_socket);
    m_client_mutex.lock();
    for (SOCKET client_socket : m_client_sockets) {
        closesocket(client_socket);
    }
    m_client_mutex.unlock();
    WSACleanup();
}