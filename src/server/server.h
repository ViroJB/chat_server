#pragma once

#include <winsock2.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <list>
#include <fmt/core.h>

class Server {
   public:
    Server();
    ~Server();
    bool Start(int port);
    bool Stop();

   private:
    bool InitializeWSA();
    SOCKET CreateServerSocket();
    bool BindServerSocket(SOCKET server_socket, sockaddr_in server_address);

    // server operations
    void StartListening();
    void StopListening();
    void AcceptConnections();
    void HandleClientMessages();
    void CheckSocketsActivity(fd_set& socket_set, SOCKET& highest_socket);
    void ProcessIncomingConnection(fd_set& socket_set);
    void ProcessClientMessages(fd_set& socket_set);
    std::string ReceiveMessageFromClient(SOCKET client_socket);
    
    void BroadcastMessage(const std::string& message, SOCKET sender_socket);

    void Cleanup();

    // member variables
    bool m_is_running;
    SOCKET m_server_socket;
    sockaddr_in m_server_address;
    std::list<SOCKET> m_client_sockets;
    std::mutex m_client_mutex;
};