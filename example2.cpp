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
#include <string.h>

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

class Login: public Event {
    public:
        Login()
            : Event(event_id::LOGIN) {
        }
        Login(Login* other)
            : Event(event_id::LOGIN) {
        }
};

class Logout: public Event {
    public:
        Logout()
            : Event(event_id::LOGOUT) {
        }
        Logout(Logout* other)
            : Event(event_id::LOGOUT) {
        }
};

class Message: public Event {
    public:
        char message[20000];

        Message()
            : Event(event_id::MESSAGE) {
        }
        Message(std::string message)
            : Event(event_id::MESSAGE) {
            strncpy(this->message, message.c_str(), 20000);
        }
        Message(Message* other)
            : Event(event_id::MESSAGE) {
            strncpy(this->message, other->message, 20000);
        }
};

Event* Event::assemble(void* buffer) {
    Event* event = reinterpret_cast<Event*>(buffer);
    EventID id = event->event_id;
    switch (id) {
        case event_id::LOGIN:
            event = new Login((Login*)buffer);
            break;
        case event_id::LOGOUT:
            event = new Logout((Logout*)buffer);
            break;
        case event_id::MESSAGE:
            event = new Message((Message*)buffer);
            break;
    };
    return event;
}


// ----------------------------------------------------------------------------

int server_thread(void* param);

class Server: public BaseServer {
    friend int server_thread(void* param);
    protected:
        void nofity(Worker* worker, Event* event);
        Worker* connect(TcpLink* link);
        void disconnect(Worker* worker);
    public:
        Server(unsigned short port);
        virtual ~Server();
        
        void logic();

        bool running;
        SDL_Thread* thread;
};

int client_thread(void* param);

class Client: public BaseClient {
    friend int client_thread(void* param);
    protected:
        void nofity(Event* event);
    public:
        Client(std::string hostname, unsigned short port);
        virtual ~Client();
        
        bool running;
        SDL_Thread* thread;
};

// ----------------------------------------------------------------------------

int client_thread(void* param) {
    Client* client = (Client*)param;
    while (client->running) {
        client->logic();
    }
    return 0;
}

int server_thread(void* param) {
    Server* server = (Server*)param;
    while (server->running) {
        server->logic();
    }
    return 0;
}

// ----------------------------------------------------------------------------

Server::Server(unsigned short port)
    : BaseServer(port) {
    running = true;
    this->thread = SDL_CreateThread(server_thread, (void*)this);
    std::cout << "Server started" << std::endl;
}

Server::~Server() {
    running = false;
    SDL_WaitThread(this->thread, NULL);
    std::cout << "Server stopped" << std::endl;
}

Worker* Server::connect(TcpLink* link) {
    Worker* worker = BaseServer::connect(link);
    if (worker != NULL) {
        std::cout << "Worker #" << worker->id << " connected" << std::endl;
    } else {
        std::cout << "No Worker got\n";
    }
    return worker;
}

void Server::logic() {
    BaseServer::logic();
    auto workers = this->workers;
    for (auto node = workers.begin(); node != workers.end(); node++) {
        BaseServer::logic(node->second);
    }
}

void Server::nofity(Worker* worker, Event* event) {
    if (worker == NULL) { std::cout << "NULL-Worker!\n"; delete event; return; }
    Message* msg = NULL;
    switch (event->event_id) {
        case event_id::MESSAGE:
            msg = (Message*)event;
            std::cout << "Worker #" << worker->id << " got: " << msg->message << std::endl;
            break;
        case event_id::LOGIN:
            std::cout << "Worker #" << worker->id << " got login" << std::endl;
            break;
        case event_id::LOGOUT:
            std::cout << "Worker #" << worker->id << " got logout" << std::endl;
            this->disconnect(worker);
            break;
        default:
            std::cout << "Worker #" << worker->id << " got unknown event-id: #" << event->event_id << std::endl;
    }
    delete event;
}

void Server::disconnect(Worker* worker) {
    std::cout << "Worker #" << worker->id << " disconnected" << std::endl;
    BaseServer::disconnect(worker);
}

Client::Client(std::string hostname, unsigned short port)
    : BaseClient(hostname, port) {
    this->running = true;
    this->thread = SDL_CreateThread(client_thread, (void*)this);
    std::cout << "Client connected" << std::endl;
}

Client::~Client() {
    this->running = false;
    SDL_WaitThread(this->thread, NULL);
    std::cout << "Client disconnected" << std::endl;
}

void Client::nofity(Event* event) {
    Message* msg = NULL;
    switch (event->event_id) {
        case event_id::MESSAGE:
            msg = (Message*)event;
            std::cout << "Client got: " << msg->message << std::endl;
            break;
        case event_id::LOGOUT:
            std::cout << "Server forces client to logout" << std::endl;
            //this->link.close();
            break;
        default:
            std::cout << "unknown event-id: #" << event->event_id << std::endl;
    }
    delete event;
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
            while (true) {
                server->logic();
                getline(std::cin, input);
                if (input == "quit") {
                    server->push(new Logout());
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
            client->push(new Login());
            while (true) {
                getline(std::cin, input);
                if (input == "quit") {
                    break;
                }
                // set message
                client->push(new Message(input));
            }
            client->push(new Logout());
            delete client;
            break;
        default:
            std::cout << "Usage:" << std::endl
                      << "\tdemo hostname port\t(start client)" << std::endl
                      << "\tdemo port\t\t(start server)" << std::endl;
    }
}


