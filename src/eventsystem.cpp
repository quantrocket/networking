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
        Event* buffer = system->outgoing.pop();
        if (buffer != NULL) {
            /*
            switch (system->link->linktype) {
                case TCP:
                    Event::toLink<TcpLink>((TcpLink*)(system->link), buffer);
                    break;
                case UDP:
                    Event::toLink<UdpLink>((UdpLink*)(system->link), buffer);
                    break;
            }
            */
            Event::toTcp(system->link, buffer);
            delete buffer;
        }
    }
    system->running = false;
    return 0;
}

int trigger_receiver(void* param) {
    NetworkingQueue* system = reinterpret_cast<NetworkingQueue*>(param);
    while (true) {
        Event* buffer = NULL;
        /*
        switch (system->link->linktype) {
            case TCP:
                buffer = Event::fromLink<TcpLink>((TcpLink*)(system->link));
                break;
            case UDP:
                buffer = Event::fromLink<UdpLink>((UdpLink*)(system->link));
                break;
        }
        */
        buffer = Event::fromTcp(system->link);
        if (buffer != NULL) {
            system->incomming.push(buffer);
        }
    }
    system->running = false;
    return 0;
}

NetworkingQueue::NetworkingQueue(TcpLink* link)
    : link(link) {
    //this->lock = SDL_CreateMutex();
    // start threads
    this->running = true;
    this->sender_thread   = SDL_CreateThread(trigger_sender,   (void*)this);
    this->receiver_thread = SDL_CreateThread(trigger_receiver, (void*)this);
}

NetworkingQueue::~NetworkingQueue() {
    // cancel threads
    SDL_KillThread(this->sender_thread);
    SDL_KillThread(this->receiver_thread);
    //SDL_DestroyMutex(this->lock);
    // note: link might be used outside -- should be deleted here
}

Event* NetworkingQueue::pop() {
    // pop from incomming queue (already thread-safe)
    return this->incomming.pop();
}

bool NetworkingQueue::isRunning() {
    return this->running;
}

bool NetworkingQueue::isEmpty() {
    return this->outgoing.isEmpty();
}

