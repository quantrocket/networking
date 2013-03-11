/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "client.hpp"

int client_handler(void* param) {
    BaseClient* client = (BaseClient*)param;
    // loop while connected to server's worker
    while (client->link.isOnline() && client->running()) {
        // handle events
        Event* next = client->queue->pop();
        if (next != NULL) {
            std::cout << "Client event\n";
            client->onEvent(next);
            delete next;
        }
    }
    return 0;
}

BaseClient::BaseClient(std::string hostname, unsigned short port)
    : _running(false) {
    this->lock = SDL_CreateMutex();
    this->link.open(hostname, port);
    this->queue = new NetworkingQueue(&(this->link));
}

BaseClient::~BaseClient() {
    this->stop();
    delete this->queue;
    SDL_DestroyMutex(this->lock);
}

void BaseClient::start() {
    if (!this->running()) {
        this->running(true);
        std::cout << "Client connect\n";
        this->onConnect();
        this->thread = SDL_CreateThread(client_handler, (void*)this);
    }
}

void BaseClient::stop() {
    if (this->running()) {
        // trigger disconnect (maybe some final information are sent)
        std::cout << "Client disconnect\n";
        this->onDisconnect();
        // wait until outgoing queue is empty
        while (!this->queue->isEmpty()) {
            SDL_Delay(DELAY_ON_EMPTY);
        }
        // wait for thread
        this->running(false);
        SDL_WaitThread(this->thread, NULL);
        this->thread = NULL;
    }
}

void BaseClient::running(bool value) {
    SDL_LockMutex(this->lock);
    this->_running = value;
    SDL_UnlockMutex(this->lock);
}

bool BaseClient::running() {
    bool value;
    SDL_LockMutex(this->lock);
    value = this->_running;
    SDL_UnlockMutex(this->lock);
    return value;
}



