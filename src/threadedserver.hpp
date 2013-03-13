/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef THREADEDSERVER_HPP
#define THREADEDSERVER_HPP

#include <vector>

#include "server.hpp"

#include <SDL/SDL.h>

int server_accepter(void* param);
int server_handler(void* param);
int server_dispatcher(void* param);

class ThreadedServer;

class Sender {
    public:
        Worker* worker;
        Event* event;
        ThreadedServer* server;

        Sender(Worker* worker, Event* event, ThreadedServer* server);
        virtual ~Sender();
};

class ThreadedServer: public BaseServer {
    friend int server_accepter(void* param);
    friend int server_handler(void* param);
    friend int server_dispatcher(void* param);
    private:
        SDL_Thread* accepter;
        SDL_Thread* handler;
        std::vector<SDL_Thread*> dispatchers;
        SDL_mutex* dispatchers_lock; // just in case of future parallelism of dispatchers-vector

        void nofity(Worker* worker, Event* event);
    protected:
        bool running;
        SDL_mutex* workers_lock;

        // @node: The given sender is deleted automatically after processing.
        virtual void dispatch(Worker* worker, Event* event) = 0;

        virtual Worker* connect(TcpLink* link);
        virtual void disconnect(Worker* worker);
    public:
        ThreadedServer(unsigned short port);
        virtual ~ThreadedServer();
};

#endif
