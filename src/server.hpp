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

#include <list>
#include <map>
#include <iostream>

#include "connection.hpp"
#include "eventsystem.hpp"

class BaseServer;

typedef unsigned short WorkerID;

class Worker {
    public:
        BaseServer* server;
        WorkerID id;
        TcpLink* link;
        NetworkingQueue* queue;
        SDL_Thread* thread;
        bool running;
};

int server_accepter(void* param);
int worker_handler(void* param);

class BaseServer {
    friend int server_accepter(void* param);
    friend int worker_handler(void* param);
    protected:
        TcpListener listener;
        // worker management
        std::map<WorkerID, Worker*> workers;
        WorkerID next_id;
        SDL_mutex* lock;
        // threading stuff
        SDL_Thread* accepter;
        bool running;

        virtual void onStart() = 0;
        virtual void onConnect(Worker* worker) = 0;
        virtual void onEvent(Worker* worker, Event* event) = 0;
        virtual void onDisconnect(Worker* worker) = 0;
        virtual void onStop() = 0;
    public:
        BaseServer(unsigned short port);
        virtual ~BaseServer();
        
        void start();
        void disconnect(Worker* worker);
        void stop();

        template <typename TEvent> void push(TEvent* event);        
        template <typename TEvent> void push(TEvent* event, WorkerID id);
};

// ----------------------------------------------------------------------------

int server_accepter(void* param) {
    BaseServer* server = (BaseServer*)param;
    while (server->running) {
        TcpLink* incomming = server->listener.accept();
        if (incomming != NULL) {
            SDL_LockMutex(server->lock);
            // create worker
            Worker* worker = new Worker();
            worker->server = server;
            worker->id     = server->next_id++;
            worker->link   = incomming;
            worker->queue  = new NetworkingQueue(worker->link);
            server->workers[worker->id] = worker;
            SDL_UnlockMutex(server->lock);
            // run worker
            worker->running = true;
            worker->thread = SDL_CreateThread(worker_handler, (void*)worker);
            std::cout << "Worker connect\n";
            server->onConnect(worker);
            SDL_UnlockMutex(server->lock);
        }
    }
    return 0;
}

int worker_handler(void* param) {
    Worker* worker = (Worker*)param;
    while (worker->running && worker->link->isOnline()) {
        Event* next = worker->queue->pop();
        if (next != NULL) {
            std::cout << "Worker event\n";
            worker->server->onEvent(worker, next);
        }
    }
    return 0;
}

BaseServer::BaseServer(unsigned short port)
    : next_id(0)
    , running(false) {
    this->listener.open(port);
}

BaseServer::~BaseServer() {
    this->stop();
    this->listener.close();
}

void BaseServer::start() {
    if (!this->running) {
        this->next_id  = 0;
        this->running  = true;
        this->accepter = SDL_CreateThread(server_accepter, (void*)this);
        std::cout << "Server start\n";
        this->onStart();
    }
}

void BaseServer::disconnect(Worker* worker) {
    // trigger disconnection event (maybe some final information are sent)
    std::cout << "Worker disconnect\n";
    this->onDisconnect(worker);
    // wait until outgoing queue is empty
    while (!worker->queue->isEmpty()) {
        SDL_Delay(DELAY_ON_EMPTY);
    }
    // wait for worker-thread
    worker->running = false;
    SDL_WaitThread(worker->thread, NULL),
    worker->thread = NULL;
}

void BaseServer::stop() {
    if (this->running) {
        // wait for accepter thread
        this->running = false;
        SDL_WaitThread(this->accepter, NULL);
        this->accepter = NULL;
        // stop workers
        for (auto node = this->workers.begin(); node != this->workers.end(); node++) {
            Worker* worker = node->second;
            if (worker->running) {
                this->disconnect(worker);
                delete worker;
            }
        }
        std::cout << "Server stop\n";
        this->onStop();
    }
}

template <typename TEvent>
void BaseServer::push(TEvent* event) {
    SDL_LockMutex(this->lock);
    for (auto node = this->workers.begin(); node != this->workers.end(); node++) {
        // push copy event to worker (deleted after sending)
        node->second->queue->push(new TEvent(*event));
    }
    SDL_UnlockMutex(this->lock);
    delete event;
}

template <typename TEvent>
void BaseServer::push(TEvent* event, WorkerID id) {
    SDL_LockMutex(this->lock);
    auto node = this->workers.find(id);
    if (node != this->workers.end()) {
        // send to worker (deleted after sending)
        node->second->queue->push(event);
    } else {
        // @note: just for debugging purpose
        std::cout << "Unknown worker id #" << id
                  << ". Pushing event canceled" << std::endl;
    }
    SDL_UnlockMutex(this->lock);
}


#endif

