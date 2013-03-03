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

template <typename TServer, typename TWorker>
int worker_handler(void* param);

/// Typeparameter TServer for Server
template <typename TServer, typename TWorker>
class BaseWorker {
    friend int worker_handler<TServer, TWorker>(void* param);
    protected:
        TcpLink* link;
        NetworkingQueue* queue;
        TServer* server;
        SDL_Thread* handle_thread;
        bool running;
        
        /// Handle given event. The event should be deleted here after handling
        virtual void handle(Event* event) = 0;
        virtual void onConnect() = 0;
        virtual void onDisconnect() = 0;
    public:
        BaseWorker(TcpLink* link, TServer* server);
        virtual ~BaseWorker();
        
        void run();
        void shutdown();
        
        template <typename TEvent>
        void push(TEvent* event);
};

// ----------------------------------------------------------------------------

template <typename TServer, typename TWorker>
int worker_handler(void* param) {
    BaseWorker<TServer, TWorker>* worker = (BaseWorker<TServer, TWorker>*)param;
    worker->onConnect();
    // loop while connected to client
    while (worker->queue->isRunning()) {
        // pop and handle event
        Event* next = worker->queue->pop();
        if (next == NULL) { continue; }
        worker->handle(next);
    }
    // remove worker from server
    worker->server->halt((TWorker*)worker);
    worker->onDisconnect();
}

template <typename TServer, typename TWorker>
BaseWorker<TServer, TWorker>::BaseWorker(TcpLink* link, TServer* server)
    : link(link)
    , server(server)
    , running(false) {
    this->queue = new NetworkingQueue(link);
}

template <typename TServer, typename TWorker>
BaseWorker<TServer, TWorker>::~BaseWorker() {
    // kill thread
    SDL_KillThread(this->handle_thread);
    delete this->queue;
    delete this->link;
}

template <typename TServer, typename TWorker>
void BaseWorker<TServer, TWorker>::run() {
    if (this->running) {
        // already running
        return;
    }
    this->running = true;
    this->handle_thread = SDL_CreateThread(worker_handler<TServer, TWorker>, (void*)this);
}

template <typename TServer, typename TWorker>
void BaseWorker<TServer, TWorker>::shutdown() {
    if (!this->running) {
        // not running
        return;
    }
    this->running = false;
    // Wait for thread (will terminate soon)
    SDL_WaitThread(this->handle_thread, NULL);
    this->handle_thread = NULL;
}

template <typename TServer, typename TWorker>
template <typename TEvent>
void BaseWorker<TServer, TWorker>::push(TEvent* event) {
    this->queue->push(event);
}

// @todo: onDisconnect()

#endif
