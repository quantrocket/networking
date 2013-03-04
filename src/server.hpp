/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef SERVER_HPP
#define SERVER_HPP

#include "connection.hpp"
#include "eventsystem.hpp"

#include <list>
#include <map>

template <typename TWorker, typename TServer>
int server_accepter(void* param);

/// Typeparameter W for Worker Class
template <typename TWorker, typename TServer>
class BaseServer {
    friend int server_accepter<TWorker, TServer>(void* param);
    protected:
        TcpListener listener;
        std::map<unsigned int, TWorker*> workers;
        unsigned int next_id;
        SDL_mutex* lock;
        // threading stuff
        SDL_Thread* accept_thread;
        SDL_Thread* handle_thread;
        bool running;
        
        void join(TcpLink* link);
        virtual void onStart() = 0;
        virtual void onStopp() = 0;
    public:
        BaseServer(unsigned short port);
        virtual ~BaseServer();
        
        void run();
        void disjoin(unsigned int id);
        void shutdown();
        
        template <typename TEvent>
        void push(TEvent* event);

        template <typename TEvent>
        void push(TEvent* event, unsigned int id);
};

// ----------------------------------------------------------------------------

template <typename TWorker, typename TServer>
int server_accepter(void* param) {
    BaseServer<TWorker, TServer>* server = (BaseServer<TWorker, TServer>*)param;
    while (server->running) {
        TcpLink* incomming = server->listener.accept();
        if (incomming == NULL) { continue; }
        server->join(incomming);        
    }
    return 0;
}

template <typename TWorker, typename TServer>
void BaseServer<TWorker, TServer>::join(TcpLink* link) {
    SDL_LockMutex(this->lock);
    // create worker
    unsigned id = this->next_id++;
    TWorker* worker = new TWorker(id, link, (TServer*)this);
    // add worker
    this->workers[id] = worker;
    SDL_UnlockMutex(this->lock);
    // run worker
    worker->run();
}

template <typename TWorker, typename TServer>
void BaseServer<TWorker, TServer>::disjoin(unsigned int id) {
    // search worker
    SDL_LockMutex(this->lock);
    auto node = this->workers.find(id);
    if (node != this->workers.end()) {
        // remove worker
        //delete node->second;
        this->workers.erase(node);
        std::cout << "Removed Worker #" << id << std::endl;
    } else {
        std::cout << "Unknown Worker #" << id << std::endl;
    }
    SDL_UnlockMutex(this->lock);
}

template <typename TWorker, typename TServer>
BaseServer<TWorker, TServer>::BaseServer(unsigned short port)
    : next_id(0)
    , running(false) {
    this->listener.open(port);
}

template <typename TWorker, typename TServer>
BaseServer<TWorker, TServer>::~BaseServer() {
    this->shutdown();
    this->listener.close();
}

template <typename TWorker, typename TServer>
void BaseServer<TWorker, TServer>::run() {
    if (this->running) {
        // already running
        return;
    }
    this->next_id = 0;
    this->running = true;
    this->accept_thread = SDL_CreateThread(server_accepter<TWorker, TServer>, (void*)this);
    this->onStart();
}

template <typename TWorker, typename TServer>
void BaseServer<TWorker, TServer>::shutdown() {
    if (!this->running) {
        // not running
        return;
    }
    // Wait for accepter thread (to away deadlock for worker lock)
    this->running = false;
    SDL_WaitThread(this->accept_thread, NULL);
    this->accept_thread = NULL;
    // Kill all worker threads
    for (auto node = this->workers.begin(); node != this->workers.end(); node++) {
        node->second->shutdown();
    }
    this->onStopp();
}

template <typename TWorker, typename TServer>
template <typename TEvent>
void BaseServer<TWorker, TServer>::push(TEvent* event) {
    SDL_LockMutex(this->lock);
    // push event to all workers
    for (auto node = this->workers.begin(); node != this->workers.end(); node++) {
        // create copy using copy constructor
        TEvent* copy = new TEvent(*event);
        // push copy to worker
        node->second->push(copy);
        // the pointer is automatically deleted after sending by the worker
    }
    SDL_UnlockMutex(this->lock);
    delete event;
}

template <typename TWorker, typename TServer>
template <typename TEvent>
void BaseServer<TWorker, TServer>::push(TEvent* event, unsigned int id) {
    SDL_LockMutex(this->lock);
    // search worker
    auto node = this->workers.find(id);
    if (node != this->workers.end()) {
        // send to worker
        node->second->push(event);
        // the pointer is automatically deleted after sending by the worker
    } else {
        std::cout << "Unknown Worker #" << id << std::endl;
    }
    SDL_UnlockMutex(this->lock);
}


#endif

