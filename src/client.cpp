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
    client->onConnect();
    // loop while connected to server's worker
    while (client->queue->isRunning()) {
        // handle events
        Event* next = client->queue->pop();
        if (next == NULL) { continue; }
        client->handle(next);
        delete next;
    }
    client->onDisconnect();
}

BaseClient::BaseClient(std::string hostname, unsigned short port)
    : running(false) {
    this->link.open(hostname, port);
    this->queue = new NetworkingQueue(&(this->link));
}

void BaseClient::run() {
    if (this->running) {
        // already running
        return;
    }
    this->running = true;
    this->handle_thread = SDL_CreateThread(client_handler, (void*)this);
}

void BaseClient::shutdown() {
    if (!this->running) {
        // not running
        return;
    }
    this->running = false;
    // Wait for thread (will terminate soon)
    SDL_WaitThread(this->handle_thread, NULL);
    this->handle_thread = NULL;
}

BaseClient::~BaseClient() {
    if (this->running) {
        // kill thread
        SDL_KillThread(this->handle_thread);
    }
    // clean queue
    delete this->queue;
}



