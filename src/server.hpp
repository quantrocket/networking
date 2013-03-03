/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the NetworkingQueueing module:
    https://github.com/cgloeckner/NetworkingQueueing

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef SERVER_HPP
#define SERVER_HPP

#include "connection.hpp"
#include "eventsystem.hpp"

#include <list>

template <typename TWorker, typename TServer>
int server_accepter(void* param);

template <typename TWorker, typename TServer>
int server_handle(void* param);

/// Typeparameter W for Worker Class
template <typename TWorker, typename TServer>
class BaseServer {
    friend int server_accepter<TWorker, TServer>(void* param);
    friend int server_handle<TWorker, TServer>(void* param);
    protected:
        TcpListener listener;
        std::list<TWorker*> workers;
        SDL_mutex* lock;
        EventQueue queue;
        // threading stuff
        SDL_Thread* accept_thread;
        SDL_Thread* handle_thread;
        bool running;
        
        void accept(TcpLink* link);
        /// Handle given event. The event should be deleted here after handling
        virtual void handle(Event* event) = 0;
        virtual void onStart() = 0;
        virtual void onStopp() = 0;
    public:
        BaseServer(unsigned short port);
        virtual ~BaseServer();
        
        void run();
        void halt(TWorker* worker);
        void shutdown();
        template <typename TEvent>
        void push(TEvent* event);
};

// ----------------------------------------------------------------------------

template <typename TWorker, typename TServer>
int server_accepter(void* param) {
    BaseServer<TWorker, TServer>* server = (BaseServer<TWorker, TServer>*)param;
    while (server->running) {
        TcpLink* incomming = server->listener.accept();
        if (incomming == NULL) { continue; }
        server->accept(incomming);        
    }
}

template <typename TWorker, typename TServer>
int server_handle(void* param) {
    BaseServer<TWorker, TServer>* server = (BaseServer<TWorker, TServer>*)param;
    while (server->running) {
        // pop and handle event
        Event* next = server->queue.pop();
        if (next == NULL) { continue; }
        server->handle(next);
    }
}

template <typename TWorker, typename TServer>
void BaseServer<TWorker, TServer>::accept(TcpLink* link) {
    // create worker
    TWorker* worker = new TWorker(link, (TServer*)this);
    SDL_LockMutex(this->lock);
    this->workers.push_back(worker);
    SDL_UnlockMutex(this->lock);
    // run worker
    worker->run();
}

template <typename TWorker, typename TServer>
void BaseServer<TWorker, TServer>::halt(TWorker* worker) {
    // stopp worker
    worker->shutdown();
    // remove worker   
    SDL_LockMutex(this->lock); 
    this->workers.remove(worker);
    SDL_UnlockMutex(this->lock);
}

template <typename TWorker, typename TServer>
BaseServer<TWorker, TServer>::BaseServer(unsigned short port)
    : running(false) {
    this->listener.open(port);
}

template <typename TWorker, typename TServer>
BaseServer<TWorker, TServer>::~BaseServer() {
    if (this->running) {
        // kill threads
        SDL_KillThread(this->accept_thread);
        SDL_KillThread(this->handle_thread);
    }
    // kill all workers
    for (auto node = this->workers.begin(); node != this->workers.end(); node++) {
        delete *node;
    }
    this->listener.close();
}

template <typename TWorker, typename TServer>
void BaseServer<TWorker, TServer>::run() {
    if (this->running) {
        // already running
        return;
    }
    this->running = true;
    this->onStart();
    this->accept_thread = SDL_CreateThread(server_accepter<TWorker, TServer>, (void*)this);
    this->handle_thread = SDL_CreateThread(server_handle<TWorker, TServer>,   (void*)this);
}

template <typename TWorker, typename TServer>
void BaseServer<TWorker, TServer>::shutdown() {
    if (!this->running) {
        // not running
        return;
    }
    this->running = false;
    // Wait for terminating threads (they should terminate very soon)
    SDL_WaitThread(this->accept_thread, NULL);
    SDL_WaitThread(this->handle_thread, NULL);
    this->accept_thread = NULL;
    this->handle_thread = NULL;
    this->onStopp();
}

template <typename TWorker, typename TServer>
template <typename TEvent>
void BaseServer<TWorker, TServer>::push(TEvent* event) {
    SDL_LockMutex(this->lock);
    // push event to all workers
    for (auto node = this->workers.begin(); node != this->workers.end(); node++) {
        // push copy using copy constructor
        // (pointer automatically deleted after sending via link)
        (*node)->push(new TEvent(*event));
    }
    SDL_UnlockMutex(this->lock);
    delete event;
}


#endif

