/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "client.hpp"


BaseClient::BaseClient(std::string hostname, unsigned short port) {
    this->link.open(hostname, port);
    this->queue = new NetworkingQueue(&(this->link));
}

BaseClient::~BaseClient() {
    // wait until outgoing queue is empty
    while (!this->queue->isEmpty()) {
        SDL_Delay(DELAY_ON_EMPTY);
    }
    // wait until disconnect by server
    while (this->link.isOnline()) {
        SDL_Delay(DELAY_ON_EMPTY);
    }
    // delete queue
    delete this->queue;
}

void BaseClient::logic() {
    if (this->link.isOnline()) {
        // handle next event
        Event* next = this->queue->pop();
        if (next != NULL) {
            this->onEvent(next);
        }
    }
}


