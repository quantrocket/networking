/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "server.hpp"

Worker::Worker(WorkerID id, TcpLink* link)
    : id(id)
    , link(link)
    , queue(new NetworkingQueue(link)) {
}

Worker::~Worker() {
    delete this->queue;
    delete this->link;
}

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
        WorkerID id = this->next_id++;
        Worker* worker = new Worker(id, link);
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
            this->nofity(worker, next);
        }
    }
}

