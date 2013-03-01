/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the NetworkingQueueing module:
    https://github.com/cgloeckner/NetworkingQueueing

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "eventsystem.hpp"

int trigger_send(void* param) {
    NetworkingQueue* system = reinterpret_cast<NetworkingQueue*>(param);
    system->send_all();
}

int trigger_recv(void* param) {
    NetworkingQueue* system = reinterpret_cast<NetworkingQueue*>(param);
    system->recv_all();
}

NetworkingQueue::NetworkingQueue(Link* link)
    : link(link)
    , running(link != NULL) {
    // start threads
    this->send_thread = SDL_CreateThread(trigger_send, (void*)this);
    this->recv_thread = SDL_CreateThread(trigger_recv, (void*)this);
}

NetworkingQueue::~NetworkingQueue() {
    // cancel threads
    SDL_KillThread(this->send_thread);
    SDL_KillThread(this->recv_thread);
    // note: link might be used outside -- should be deleted here
}

void NetworkingQueue::send_all() {
    while (this->running) {
        // pop from outgoing queue
        SDL_LockMutex(this->lock);
        if (this->outgoing.empty()) {
            SDL_UnlockMutex(this->lock);
            SDL_Delay(15);
            continue;
        }
        Event* next = this->outgoing.front();
        this->outgoing.pop();
        int size = this->size.front();
        this->size.pop();
        SDL_UnlockMutex(this->lock);
        // send via link
        this->link->send(next, size);
    }
}

void NetworkingQueue::recv_all() {
    while (this->running) {
        // receive via link
        void* next = this->link->receive();
        if (next == NULL) {
            SDL_Delay(15);
            continue;
        }
        // push to incomming queue (already thread-safe)
        this->incomming.push(reinterpret_cast<Event*>(next));
    }
}

Event* NetworkingQueue::pop() {
    // pop from incomming queue (already thread-safe)
    return this->incomming.pop();
}


