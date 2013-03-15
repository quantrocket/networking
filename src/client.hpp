/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <queue>

#include "threading.hpp"
#include "connection.hpp"
#include "eventsystem.hpp"

int client(void* param);

class ServerData {
    public:
        Event* event;
        std::size_t size;
};

class Client {
    friend int client(void* param);
    private:
        TcpLink link;
        Thread thread;
        // data management
        std::queue<ServerData*> outgoing;
        Mutex send;
        std::queue<ServerData*> incomming;
        Mutex recv;
        
        void logic();
    public:
        // connection management
        Client();
        virtual ~Client();
        void connect(const std::string& ip, unsigned short port);
        bool isOnline();
        void disconnect(bool immediately=false);
        // data management
        ServerData* pop();
        template <typename TEvent> void push(TEvent* event) {
            // create bundle
            ServerData* bundle = new ServerData();
            bundle->event = event;
            bundle->size  = sizeof(TEvent);
            // push to queue
            this->send.lock();
            this->outgoing.push(bundle);
            this->send.unlock();
        } 
};


#endif

