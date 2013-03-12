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
        Event* event = system->outgoing.pop();
        if (event != NULL) {
            std::size_t len = system->outlen.front();
            system->outlen.pop();
            try {
                system->link->send_data<std::size_t>(len);
                system->link->send_ptr(event, len);
                //Event::toTcp(system->link, buffer);
                delete event;
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
        void* buffer = NULL;
        try {
            std::size_t len = system->link->receive_data<std::size_t>();
            if (len > 0) {
                // read data
                buffer = system->link->receive_ptr(len);
                if (buffer != NULL) {
                    Event* event = Event::assemble(buffer);
                    if (event != NULL) {
                        system->incomming.push(event);
                    }
                    free(buffer);
                }
            }
            //buffer = Event::fromTcp(system->link);
        } catch (const BrokenPipe& e) {
            system->running = false;
        }
    }
    return 0;
}

NetworkingQueue::NetworkingQueue(TcpLink* link)
    : link(link) {
    this->outlock = SDL_CreateMutex();
    // start threads
    this->running = true;
    this->sender_thread   = SDL_CreateThread(trigger_sender,   (void*)this);
    this->receiver_thread = SDL_CreateThread(trigger_receiver, (void*)this);
}

NetworkingQueue::~NetworkingQueue() {
    SDL_DestroyMutex(this->outlock);
    // cancel threads
    this->running = false;
    this->link->close();
    SDL_WaitThread(this->sender_thread, NULL);
    SDL_KillThread(this->receiver_thread); // SDLNet_TCP_Recv is blocking
    //SDL_WaitThread(this->receiver_thread, NULL);
    // note: link might be used outside -- should be deleted here
}

Event* NetworkingQueue::pop() {
    // pop from incomming queue (already thread-safe)
    return this->incomming.pop();
}

bool NetworkingQueue::isEmpty() {
    return this->outgoing.isEmpty();
}

