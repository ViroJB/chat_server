#include "app.h"

// TODO
// handle connections: accept and close connections, handle lost connections?
// receive and send messages

App::App() {
    fmt::print("Server starting...\n");

    Server server;
    if (!server.Start(8080)) {
        fmt::print("Failed to start server.\n");
        return;
    }

    fmt::print("Server started. Press Enter to stop.\n");
    std::cin.get();

    server.Stop();

    fmt::print("Server stopped.\n");
}

int main() {
    App* app = new App();
    return 0;
}