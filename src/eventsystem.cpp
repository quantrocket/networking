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
    while (system->running) {
        Event* buffer = system->outgoing.pop();
        if (buffer != NULL) {
            try {
                Event::toTcp(system->link, buffer);
                delete buffer;
            } catch (const BrokenPipe& e) {
                system->running = false;
            }
        }
    }
    return 0;
}

int trigger_receiver(void* param) {
    NetworkingQueue* system = reinterpret_cast<NetworkingQueue*>(param);
    while (system->running) {
        Event* buffer = NULL;
        try {
            buffer = Event::fromTcp(system->link);
        } catch (const BrokenPipe& e) {
            system->running = false;
        }
        if (buffer != NULL) {
            system->incomming.push(buffer);
        }
    }
    return 0;
}

NetworkingQueue::NetworkingQueue(TcpLink* link)
    : link(link) {
    // start threads
    this->running = true;
    this->sender_thread   = SDL_CreateThread(trigger_sender,   (void*)this);
    this->receiver_thread = SDL_CreateThread(trigger_receiver, (void*)this);
}

NetworkingQueue::~NetworkingQueue() {
    // cancel threads
    this->running = false;
    this->link->close();
    SDL_WaitThread(this->sender_thread, NULL);
    SDL_WaitThread(this->receiver_thread, NULL);
    // note: link might be used outside -- should be deleted here
}

Event* NetworkingQueue::pop() {
    // pop from incomming queue (already thread-safe)
    return this->incomming.pop();
}

bool NetworkingQueue::isEmpty() {
    return this->outgoing.isEmpty();
}

