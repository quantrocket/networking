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

#include "connection.hpp"
#include "eventsystem.hpp"

int client_handler(void* param);

class BaseClient {
    friend int client_handler(void* param);
    protected:
        TcpLink link;
        NetworkingQueue* queue; // events to send
        SDL_Thread* thread;
        bool _running;
        SDL_mutex* lock; // for running

        virtual void onConnect() = 0;
        virtual void onEvent(Event* event) = 0;
        virtual void onDisconnect() = 0;
    public:
        BaseClient(std::string hostname, unsigned short port);
        virtual ~BaseClient();
        
        void start();
        void stop();

        template <typename TEvent> void push(TEvent* event);
        
        void running(bool value);
        bool running();
};

template <typename TEvent>
void BaseClient::push(TEvent* event) {
    this->queue->push(event);
}


#endif

