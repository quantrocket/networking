/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef WORKER_HPP
#define WORKER_HPP

#include "connection.hpp"
#include "eventsystem.hpp"

template <typename TServer>
int worker_handler(void* param);

/// Typeparameter TServer for Server
template <typename TServer>
class BaseWorker {
    friend int worker_handler<TServer>(void* param);
    protected:
        unsigned int id;
        TcpLink* link;
        NetworkingQueue* queue; // received events
        TServer* server;
        SDL_Thread* handle_thread;
        bool running;
        
        /// Handle given event. The event should be deleted here after handling
        virtual void handle(Event* event) = 0;
        virtual void onConnect() = 0;
        virtual void onDisconnect() = 0;
    public:
        BaseWorker(unsigned int id, TcpLink* link, TServer* server);
        virtual ~BaseWorker();
        
        void run();
        void shutdown();
        unsigned int getId();
        
        template <typename TEvent>
        void push(TEvent* event);
};

// ----------------------------------------------------------------------------

template <typename TServer>
int worker_handler(void* param) {
    BaseWorker<TServer>* worker = (BaseWorker<TServer>*)param;
    // loop while connected to client
    while (worker->queue->isRunning() && worker->running) {
        // pop and handle event
        Event* next = worker->queue->pop();
        if (next == NULL) { continue; }
        worker->handle(next);
    }
    return 0;
}

template <typename TServer>
BaseWorker<TServer>::BaseWorker(unsigned int id, TcpLink* link, TServer* server)
    : id(id)
    , link(link)
    , server(server)
    , running(false) {
    this->queue = new NetworkingQueue(link);
}

template <typename TServer>
BaseWorker<TServer>::~BaseWorker() {
    this->shutdown();
    delete this->queue;
    delete this->link;
}

template <typename TServer>
void BaseWorker<TServer>::run() {
    if (this->running) {
        // already running
        return;
    }
    this->running = true;
    this->onConnect();
    this->handle_thread = SDL_CreateThread(worker_handler<TServer>, (void*)this);
}

template <typename TServer>
void BaseWorker<TServer>::shutdown() {
    if (!this->running) {
        // not running
        return;
    }
    // wait until outgoing queue is empty
    while (!this->queue->isEmpty()) {
        SDL_Delay(DELAY_ON_EMPTY);
    }
    // wait for thread
    this->running = false;
    SDL_WaitThread(this->handle_thread, NULL);
    this->handle_thread = NULL;
    // remove worker from server
    this->onDisconnect();
    this->server->disjoin(this->id);
}

template <typename TServer>
template <typename TEvent>
void BaseWorker<TServer>::push(TEvent* event) {
    this->queue->push(event);
}

template <typename TServer>
unsigned int BaseWorker<TServer>::getId() {
    return this->id;
}

#endif
