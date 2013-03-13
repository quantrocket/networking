/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "threadedserver.hpp"

int server_handler(void* param) {
    ThreadedServer* server = (ThreadedServer*)param;
    while (server->running) {
        // Create copy of workers-map, because each connect/disconnect will
        // modify the original map. So proper iteration would be disturbed.
        SDL_LockMutex(server->workers_lock);
        auto workers = server->workers;
        SDL_UnlockMutex(server->workers_lock);
        for (auto node = workers.begin(); node != workers.end(); node++) {
            server->logic(node->second);
        }
    }
    return 0;
}

int server_accepter(void* param) {
    ThreadedServer* server = (ThreadedServer*)param;
    while (server->running) {
        server->logic();
    }
    return 0;
}

int server_dispatcher(void* param) {
    Sender* sender = (Sender*)param;
    sender->server->dispatch(sender->worker, sender->event);
    delete sender;
    std::cout << "dispatch finished\n";
}

// ---------------------------------------------------------------------------- 

Sender::Sender(Worker* worker, Event* event, ThreadedServer* server)
    : worker(worker)
    , event(event)
    , server(server) {
}

Sender::~Sender() {
    delete this->event;
    // @note: The given worker is used anymore
}

// ---------------------------------------------------------------------------- 

ThreadedServer::ThreadedServer(unsigned short port)
    : BaseServer(port) {
    this->workers_lock    = SDL_CreateMutex();
    this->dispatchers_lock = SDL_CreateMutex();
    running = true;
    this->accepter = SDL_CreateThread(server_accepter, (void*)this);
    this->handler  = SDL_CreateThread(server_handler,  (void*)this);
}

ThreadedServer::~ThreadedServer() {
    running = false;
    // wait for accepter and handler threads
    SDL_WaitThread(this->accepter, NULL);
    SDL_WaitThread(this->handler,  NULL);
    // wait for all dispatcher threads (as a rule the majority was been finished, yet)
    for (auto node = this->dispatchers.begin(); node != this->dispatchers.end(); node++) {
        SDL_WaitThread(*node, NULL);
    }
    // free mutices
    SDL_DestroyMutex(this->workers_lock);
    SDL_DestroyMutex(this->dispatchers_lock);
}

void ThreadedServer::nofity(Worker* worker, Event* event) {
    // sender is deleted at the end of server_dispatcher()
    Sender* sender = new Sender(worker, event, this);
    SDL_Thread* thread = SDL_CreateThread(server_dispatcher, (void*)sender);
    // when destroying the server, it will wait for all dispatchers
    SDL_LockMutex(this->dispatchers_lock);
    this->dispatchers.push_back(thread);
    SDL_UnlockMutex(this->dispatchers_lock);
}

Worker* ThreadedServer::connect(TcpLink* link) {
    SDL_LockMutex(this->workers_lock);
    Worker* worker = BaseServer::connect(link);
    SDL_UnlockMutex(this->workers_lock);
    return worker;
}

void ThreadedServer::disconnect(Worker* worker) {
    SDL_LockMutex(this->workers_lock);
    BaseServer::disconnect(worker);
    SDL_UnlockMutex(this->workers_lock);
}

