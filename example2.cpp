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

    Message()
        : Event(event_id::MESSAGE) {
    }
    Message(std::string message)
        : Event(event_id::MESSAGE) {
        memcpy(this->message, message.c_str(), 20000);
    }
    Message(const Message& other)
        : Event(other.event_id) {
        memcpy(this->message, other.message, 20000);
    }
};

void Event::toTcp(TcpLink* link, Event* event) {
    // send event id
    EventID id = event->event_id;
    link->send<EventID>(&id);
    // send event data
    switch (id) {
        case event_id::LOGIN:
            link->send<Login>(event);
            break;
        case event_id::LOGOUT:
            link->send<Logout>(event);
            break;
        case event_id::MESSAGE:
            link->send<Message>(event);
            break;
    }
}

Event* Event::fromTcp(TcpLink* link) {
    Event* event = NULL;
    // receive event id
    EventID* id = link->receive<EventID>();
    // receive event
    switch (*id) {
        case event_id::LOGIN:
            event = link->receive<Login>();
            break;
        case event_id::LOGOUT:
            event = link->receive<Logout>();
            break;
        case event_id::MESSAGE:
            event = link->receive<Message>();
            break;
    }
    return event;
}

// ----------------------------------------------------------------------------

class Server: public BaseServer {
    protected:
        void onStart();
        void onConnect(Worker* worker);
        void onEvent(Worker* worker, Event* event);
        void onDisconnect(Worker* worker);
        void onStop();
    public:
        Server(unsigned short port);
};

class Client: public BaseClient {
    protected:
        void onEvent(Event* event);
        void onConnect();
        void onDisconnect();
    public:
        Client(std::string hostname, unsigned short port);
};

// ----------------------------------------------------------------------------

Server::Server(unsigned short port)
    : BaseServer(port) {
}

void Server::onStart() {
    std::cout << "Server started" << std::endl;
}

void Server::onConnect(Worker* worker) {
    std::cout << "Worker #" << worker->id << " connected" << std::endl;
}

void Server::onEvent(Worker* worker, Event* event) {
    if (event->event_id == event_id::MESSAGE) {
        Message* msg = (Message*)event;
        std::cout << "Worker #" << worker->id << " got: " << msg->message << std::endl;
    } else if (event->event_id == event_id::LOGIN) {
        std::cout << "Worker #" << worker->id << " got login" << std::endl;
    } else if (event->event_id == event_id::LOGOUT) {
        std::cout << "Worker #" << worker->id << " got logout" << std::endl;
        worker->server->disconnect(worker);
    } else {
        std::cout << "Worker #" << worker->id << " got unknown event-id: #" << event->event_id << std::endl;
    }
}

void Server::onDisconnect(Worker* worker) {
    std::cout << "Worker #" << worker->id << " disconnected" << std::endl;
}

void Server::onStop() {
    std::cout << "Server stopped" << std::endl;
}

Client::Client(std::string hostname, unsigned short port)
    : BaseClient(hostname, port) {
}

void Client::onConnect() {
    std::cout << "Client connected" << std::endl;
}

void Client::onEvent(Event* event) {
    if (event->event_id == event_id::MESSAGE) {
        Message* msg = (Message*)event;
        std::cout << "Client got: " << msg->message << std::endl;
    } else if (event->event_id == event_id::LOGOUT) {
        std::cout << "Server forces client to logout" << std::endl;
        this->stop();
    } else {
        std::cout << "unknown event-id: #" << event->event_id << std::endl;
    }
}

void Client::onDisconnect() {
    std::cout << "Client disconnected" << std::endl;
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
            server->start();
            while (true) {
                getline(std::cin, input);
                if (input == "restart") {
                    server->push(new Logout());
                    server->stop();
                    server->start();
                    continue;
                }
                if (input == "quit") {
                    server->push(new Logout());
                    server->stop();
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
            client->start();
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
            client->stop();
            delete client;
            break;
        default:
            std::cout << "Usage:" << std::endl
                      << "\tdemo hostname port\t(start client)" << std::endl
                      << "\tdemo port\t\t(start server)" << std::endl;
    }
}


