/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "client.hpp"

namespace networking {

    int client(void* param) {
        Client* client = (Client*) param;
        client->logic();
        return 0;
    }

    Client::Client() {
    }

    Client::~Client() {
        this->disconnect(true);
    }

    void Client::logic() {
        while (this->isOnline()) {
            // try send all outgoing bundles
            this->send.lock();
            while (!this->outgoing.empty()) {
                // pop bundle
                ServerData* bundle = this->outgoing.front();
                this->outgoing.pop();
                if (bundle == NULL) {
                    continue;
                }
                // send
                try {
                    this->link.send_data<std::size_t>(bundle->size);
                    this->link.send_ptr(bundle->event, bundle->size);
                } catch (const BrokenPipe& broken_pipe) {
                    // connection to server lost
                    this->send.unlock();
                    // discard bundle
                    delete bundle->event;
                    delete bundle;
                    return;
                }
            }
            this->send.unlock();
            //  try receive some incomming bundles
            int recv_limit = 10; // @todo: use client->recv_limit;
            do {
                recv_limit--;
                if (this->link.isReady()) {
                    ServerData* bundle = new ServerData();
                    try {
                        bundle->size = this->link.receive_data<std::size_t>();
                    } catch (const BrokenPipe& broken_pipe) {
                        // connection to server lost
                        delete bundle;
                        return;
                    }
                    if (bundle->size == 0) {
                        // no furthur data expected
                        delete bundle;
                        continue;
                    }
                    void* buffer = NULL;
                    try {
                        buffer = this->link.receive_ptr(bundle->size);
                    } catch (const BrokenPipe& broken_pipe) {
                        // connection to server lost
                        delete bundle;
                        return;
                    }
                    if (buffer == NULL) {
                        // no data got
                        free(buffer);
                        delete bundle;
                        continue;
                    }
                    // re-assemble event data
                    bundle->event = Event::assemble(buffer);
                    free(buffer);
                    if (bundle->event == NULL) {
                        // no event assembled
                        continue;
                    }
                    // add bundle to system
                    this->recv.lock();
                    this->incomming.push(bundle);
                    this->recv.unlock();
                }
            } while (recv_limit >= 0);
        }
        SDL_Delay(100);
    }

    void Client::connect(const std::string& ip, unsigned short port) {
        if (this->isOnline()) {
            return;
        }
        // open connection
        this->link.open(ip, port);
        this->thread.run(client, (void*)this);
    }

    bool Client::isOnline() {
        return this->link.isOnline();
    }

    void Client::disconnect(bool immediately) {
        if (!this->isOnline()) {
            return;
        }
        // close connection
        this->link.close();
        if (immediately == true) {
            this->thread.kill();
        } else {
            this->thread.wait();
        }
    }

    ServerData* Client::pop() {
        ServerData* tmp = NULL;
        this->recv.lock();
        if (!this->incomming.empty()) {
            tmp = this->incomming.front();
            this->incomming.pop();
        }
        this->recv.unlock();
        return tmp;
    }

}

