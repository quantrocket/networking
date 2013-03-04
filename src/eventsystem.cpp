/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "eventsystem.hpp"

int trigger_sender(void* param) {
    NetworkingQueue* system = reinterpret_cast<NetworkingQueue*>(param);
    while (true) {
        // pop from outgoing queue
        SDL_LockMutex(system->lock);
        if (system->outgoing.empty()) {
            SDL_UnlockMutex(system->lock);
            SDL_Delay(DELAY_ON_EMPTY);
            continue;
        }
        Event* next = system->outgoing.front();
        system->outgoing.pop();
        int size = system->size.front();
        system->size.pop();
        SDL_UnlockMutex(system->lock);
        // push to network
        try {
            // send via link
            system->link->send(next, size);
        } catch (const BrokenPipe e) {
            system->link->close();
            delete next;
            break;
        }
        delete next;
    }
    system->running = false;
    return 0;
}

int trigger_receiver(void* param) {
    NetworkingQueue* system = reinterpret_cast<NetworkingQueue*>(param);
    while (true) {
        // pop from networking
        void* next = NULL;
        try {
            // receive via link
            next = system->link->receive();
        } catch (const BrokenPipe e) {
            system->link->close();
            break;
        }
        if (next == NULL) {
            SDL_Delay(DELAY_ON_EMPTY);
            continue;
        }
        // push to incomming queue (already thread-safe)
        system->incomming.push(reinterpret_cast<Event*>(next));
    }
    system->running = false;
    return 0;
}

NetworkingQueue::NetworkingQueue(Link* link)
    : link(link) {
    this->lock = SDL_CreateMutex();
    // start threads
    this->running = true;
    this->sender_thread   = SDL_CreateThread(trigger_sender,   (void*)this);
    this->receiver_thread = SDL_CreateThread(trigger_receiver, (void*)this);
}

NetworkingQueue::~NetworkingQueue() {
    // cancel threads
    SDL_KillThread(this->sender_thread);
    SDL_KillThread(this->receiver_thread);
    // note: link might be used outside -- should be deleted here
    SDL_DestroyMutex(this->lock);
}

Event* NetworkingQueue::pop() {
    // pop from incomming queue (already thread-safe)
    return this->incomming.pop();
}

bool NetworkingQueue::isRunning() {
    return this->running;
}

bool NetworkingQueue::isEmpty() {
    return this->outgoing.empty();
}


