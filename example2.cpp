/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include <iostream>
#include <vector>

#include "src/connection.hpp"
#include "src/eventsystem.hpp"
#include "src/server.hpp"
#include "src/worker.hpp"
#include "src/client.hpp"

// ----------------------------------------------------------------------------

namespace event_id {
    const EventID MESSAGE = 1;
}

struct Message: Event {
    char message[20000];

    Message(std::string message)
        : Event(event_id::MESSAGE) {
        memcpy(this->message, message.c_str(), 20000);
    }
    Message(const Message& other)
        : Event(other.event_id) {
        memcpy(this->message, other.message, 20000);
    }
};

// ----------------------------------------------------------------------------

class Server;

class Worker: public BaseWorker<Server, Worker> {
    protected:
        void handle(Event* event);
        void onConnect();
        void onDisconnect();
    public:
        Worker(TcpLink* link, Server* server);
};

class Server: public BaseServer<Worker, Server> {
    protected:
        void handle(Event* event);
        void onStart();
        void onStopp();
    public:
        Server(unsigned short port);
};

class Client: public BaseClient {
    protected:
        void handle(Event* event);
        void onConnect();
        void onDisconnect();
    public:
        Client(std::string hostname, unsigned short port);
};

// ----------------------------------------------------------------------------

void Worker::handle(Event* event) {
    Message* msg = (Message*)event;
    std::cout << "Worker got: " << msg->message << std::endl;
}

void Worker::onConnect() {
    std::cout << "Worker connected" << std::endl;
}

void Worker::onDisconnect() {
    std::cout << "Worker disconnected" << std::endl;
}

Worker::Worker(TcpLink* link, Server* server)
    : BaseWorker<Server, Worker>(link, server) {
}

void Server::handle(Event* event) {
    Message* msg = (Message*)event;
    std::cout << "Server got: " << msg->message << std::endl;
}

void Server::onStart() {
    std::cout << "Server started" << std::endl;
}

void Server::onStopp() {
    std::cout << "Server stopped" << std::endl;
}

Server::Server(unsigned short port)
    : BaseServer<Worker, Server>(port) {
}

void Client::handle(Event* event) {
    Message* msg = (Message*)event;
    std::cout << "Client got: " << msg->message << std::endl;
}

void Client::onConnect() {
    std::cout << "Client connected" << std::endl;
}

void Client::onDisconnect() {
    std::cout << "Client disconnected" << std::endl;
}

Client::Client(std::string hostname, unsigned short port)
    : BaseClient(hostname, port) {
}

// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
    std::string input;

    Server* server = NULL;
    Client* client = NULL;

    switch (argc) {
        case 2:
            server = new Server((unsigned short)(atoi(argv[1])));
            server->run();
            while (true) {
                getline(std::cin, input);
                if (input == "quit") {
                    break;
                }
                // set message
                server->push(new Message(input));
            }
            delete server;
            break;
        case 3:
            client = new Client(argv[1], (unsigned short)(atoi(argv[2])));
            client->run();
            while (true) {
                getline(std::cin, input);
                if (input == "quit") {
                    break;
                }
                // set message
                client->push(new Message(input));
            }
            delete client;
            break;
        default:
            std::cout << "Usage:" << std::endl
                      << "\tdemo hostname port\t(start client)" << std::endl
                      << "\tdemo port\t\t(start server)" << std::endl;
            return EXIT_FAILURE;
    }

}


