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
        WorkerID id;
        TcpLink* link;
        NetworkingQueue* queue;
};

/// Base Server
/**
 *  The class provides primitive non-threaded Server-Client communication.
 *  Each client is handled by a worker using an id, the tcp link and the
 *  networking queue.
 *
 *  To trigger accepting new clients you can use
 *      virtual void logic()
 *  To trigger a worker's logic you can use
 *      virtual void logic(Worker* worker)
 *
 *  Remember to add mutex/locking then using threads. The call of connect()
 *  and disconnect() should be thread-safe referring to the call of a worker's
 *  logic.
 *
 *  If you want to iterate through all workers to call their' logics using a
 *  thread, you should iterate through a copy of the workers-map, because
 *  disconnect() will remove the worker from the map. So you will crash using
 *  an iterator on the original map because your current worker-node
 *  will be invalid after disconnect()'ing him. You will not be able to iterate
 *  to the next worker properly. A copy of the map would avoid this, because
 *  you will iterate through a previously made copy. So the original map can be
 *  manipulated without effecting your current iteration.
 *  This might also be relevant for seperat accepting-clients-threads and the
 *  call of connect(), which will create a worker and append him to the map.
 */
class BaseServer {
    protected:
        TcpListener listener;
        // worker management
        std::map<WorkerID, Worker*> workers;
        WorkerID next_id;

        virtual void onEvent(Worker* worker, Event* event) = 0;
        virtual Worker* connect(TcpLink* link);
        virtual void disconnect(Worker* worker);

    public:
        BaseServer(unsigned short port);
        virtual ~BaseServer();

        virtual void logic();
        virtual void logic(Worker* worker);

        template <typename TEvent> void push(TEvent* event);        
        template <typename TEvent> void push(TEvent* event, WorkerID id);
};

// ----------------------------------------------------------------------------

BaseServer::BaseServer(unsigned short port)
    : next_id(0) {
    this->listener.open(port);
}

BaseServer::~BaseServer() {
    // disconnect workers
    for (auto node = this->workers.begin(); node != this->workers.end(); node++) {
        if (node->second != NULL) {
            this->disconnect(node->second);
        }
    }
    this->listener.close();
}

Worker* BaseServer::connect(TcpLink* link) {
    if (link != NULL) {
        // create worker
        Worker* worker = new Worker();
        worker->id     = this->next_id++;
        worker->link   = link;
        worker->queue  = new NetworkingQueue(worker->link);
        this->workers[worker->id] = worker;
        return worker;
    } else {
        return NULL;
    }
}

void BaseServer::disconnect(Worker* worker) {
    if (worker != NULL) {
        // wait until outgoing queue is empty
        while (!worker->queue->isEmpty()) {
            SDL_Delay(DELAY_ON_EMPTY);
        }
        // delete and remove worker
        delete worker->queue;
        delete worker->link;
        this->workers.erase(this->workers.find(worker->id));
        delete worker;
    }
}

void BaseServer::logic() {
    TcpLink* incomming = this->listener.accept();
    if (incomming != NULL) {
        this->connect(incomming);
    }
}

void BaseServer::logic(Worker* worker) {
    if (worker != NULL && worker->link->isOnline()) {
        Event* next = worker->queue->pop();
        if (next != NULL) {
            this->onEvent(worker, next);
        }
    }
}

template <typename TEvent>
void BaseServer::push(TEvent* event) {
    if (event != NULL) {
        for (auto node = this->workers.begin(); node != this->workers.end(); node++) {
            if (node->second != NULL) {
                // push event copy to worker (deleted after sending)
                node->second->queue->push(new TEvent(event));
            }
        }
        delete event;
    }
}

template <typename TEvent>
void BaseServer::push(TEvent* event, WorkerID id) {
    if (event != NULL) {
        auto node = this->workers.find(id);
        if (node != this->workers.end() && node->second != NULL) {
            // send to worker (deleted after sending)
            node->second->queue->push(event);
        } else {
            // @note: just for debugging purpose
            std::cout << "Unknown worker id #" << id
                      << ". Pushing event canceled" << std::endl;
        }
    }
}


#endif

