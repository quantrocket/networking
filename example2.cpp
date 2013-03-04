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
    const EventID LOGIN   = 1;
    const EventID LOGOUT  = 2;
    const EventID MESSAGE = 3;
}

struct Login: Event {
    Login()
        : Event(event_id::LOGIN) {
    }
};

struct Logout: Event {
    Logout()
        : Event(event_id::LOGOUT) {
    }
};

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

class Worker: public BaseWorker<Server> {
    protected:
        void handle(Event* event);
        void onConnect();
        void onDisconnect();
    public:
        Worker(unsigned int id, TcpLink* link, Server* server);
};

class Server: public BaseServer<Worker, Server> {
    protected:
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
    if (event->event_id == event_id::MESSAGE) {
        Message* msg = (Message*)event;
        std::cout << "Worker #" << this->id << " got: " << msg->message << std::endl;
    } else if (event->event_id == event_id::LOGIN) {
        std::cout << "Worker #" << this->id << " got login" << std::endl;
    } else if (event->event_id == event_id::LOGOUT) {
        std::cout << "Worker #" << this->id << " got logout" << std::endl;
        this->shutdown();
    } else {
        std::cout << "Worker #" << this->id << " got unknown event-id: #" << event->event_id << std::endl;
    }
}

void Worker::onConnect() {
    std::cout << "Worker #" << this->id << " connected" << std::endl;
}

void Worker::onDisconnect() {
    std::cout << "Worker #" << this->id << " disconnected" << std::endl;
}

Worker::Worker(unsigned int id, TcpLink* link, Server* server)
    : BaseWorker<Server>(id, link, server) {
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
    if (event->event_id == event_id::MESSAGE) {
        Message* msg = (Message*)event;
        std::cout << "Client got: " << msg->message << std::endl;
    } else if (event->event_id == event_id::LOGOUT) {
        std::cout << "Server forces client to logout" << std::endl;
        this->shutdown();
    } else {
        std::cout << "unknown event-id: #" << event->event_id << std::endl;
    }
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
    std::string rest;

    Server* server = NULL;
    Client* client = NULL;
    bool all = false;
    unsigned int target = 0;

    switch (argc) {
        case 2:
            server = new Server((unsigned short)(atoi(argv[1])));
            server->run();
            while (true) {
                getline(std::cin, input);
                if (input == "restart") {
                    server->push(new Logout());
                    server->shutdown();
                    server->run();
                    continue;
                }
                if (input == "quit") {
                    server->push(new Logout());
                    server->shutdown();
                    break;
                }
                if (input[0] == 't' && input[1] == 'o') {
                    // set new target
                    rest = input.substr(3, input.size()-3);
                    if (rest == "all") {
                        all = true;
                    } else {
                        all = false;
                        target = (unsigned int)(std::stoi(rest));
                    }
                } else if (target >= 0 && !all) {
                    // set message
                    server->push(new Message(input), target);
                } else if (all) {
                    server->push(new Message(input));
                }
            }
            delete server;
            break;
        case 3:
            client = new Client(argv[1], (unsigned short)(atoi(argv[2])));
            client->run();
            client->push(new Login());
            while (client->running()) {
                getline(std::cin, input);
                if (input == "quit") {
                    break;
                }
                // set message
                client->push(new Message(input));
            }
            client->push(new Logout());
            client->shutdown();
            delete client;
            break;
        default:
            std::cout << "Usage:" << std::endl
                      << "\tdemo hostname port\t(start client)" << std::endl
                      << "\tdemo port\t\t(start server)" << std::endl;
    }
}


