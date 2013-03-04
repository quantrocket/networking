/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "client.hpp"

#include <iostream>

int client_handler(void* param) {
    BaseClient* client = (BaseClient*)param;
    // loop while connected to server's worker
    while (client->queue->isRunning() && client->running()) {
        // handle events
        Event* next = client->queue->pop();
        if (next == NULL) { continue; }
        client->handle(next);
        delete next;
    }
    return 0;
}

BaseClient::BaseClient(std::string hostname, unsigned short port)
    : _running(false) {
    this->lock = SDL_CreateMutex();
    this->link.open(hostname, port);
    this->queue = new NetworkingQueue(&(this->link));
}

void BaseClient::run() {
    if (this->running()) {
        // already running
        return;
    }
    this->running(true);
    this->onConnect();
    this->handle_thread = SDL_CreateThread(client_handler, (void*)this);
}

void BaseClient::shutdown() {
    if (!this->running()) {
        // not running
        return;
    }
    // wait until outgoing queue is empty
    while (!this->queue->isEmpty()) {
        SDL_Delay(DELAY_ON_EMPTY);
    }
    // wait for thread
    this->running(false);
    SDL_WaitThread(this->handle_thread, NULL);
    this->handle_thread = NULL;

    this->onDisconnect();
}

BaseClient::~BaseClient() {
    this->shutdown();
    delete this->queue;
    SDL_DestroyMutex(this->lock);
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



