#include <iostream>

#include "example/server.hpp"
#include "example/client.hpp"

int main() {
    EventPipe* a = new EventPipe();
    EventPipe* b = new EventPipe();
    Server* server = new Server(a, b);
    Client* client = new Client(b, a);

    std::string input;
    while (client->running) {
        std::getline(std::cin, input);
        if (!client->authed) {
            client->do_login(input);
        } else if (input == "/q") {
            client->do_logout();
        } else {
            client->do_message(input);
        }
    }

    delete server;
    delete client;
    delete a;
    delete b;
}


