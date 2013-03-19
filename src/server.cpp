/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "server.hpp"

namespace networking {

    void server(Server* server) {
        server->logic();
    }

    Server::Server(short max_clients)
        : max_clients(max_clients)
        , next_id(0) {
    }

    Server::~Server() {
        this->shutdown();
    }

    void Server::logic() {
        while (this->isOnline()) {
            // try accept the next client
            TcpLink* next_link = this->listener.accept();
            if (next_link != NULL) {
                // add worker
                this->workers.lock();
                ClientID id = this->next_id++;
                this->links[id] = next_link;
                this->workers.unlock();
            }
            // try send all outgoing bundles
            this->send.lock();
            while (!this->outgoing.empty()) {
                // pop bundle
                ClientData* bundle = this->outgoing.front();
                this->outgoing.pop();
                if (bundle == NULL) {
                    continue;
                }
                // get link
                this->workers.lock();
                TcpLink* link = NULL;
                auto node = this->links.find(bundle->id);
                if (node != this->links.end() && node->second != NULL && node->second->isOnline()) {
                    link = node->second;
                } else {
                    // target not found
                    this->workers.unlock();
                    // discard bundle
                    delete bundle->event;
                    delete bundle;
                    continue;
                }
                // send
                try {
                    link->send_data<std::size_t>(bundle->size);
                    link->send_ptr(bundle->event, bundle->size);
                } catch (const BrokenPipe& broken_pipe) {
                    // connection to worker lost
                    this->workers.unlock();
                    // disconnect worker
                    this->disconnect(bundle->id);
                    // discard bundle
                    delete bundle->event;
                    delete bundle;
                }
                this->workers.unlock();
            }
            this->send.unlock();
            // try receive some incomming bundles
            this->workers.lock();
            auto node = this->links.begin();
            while (node != this->links.end()) {
                if (node->second == NULL || !node->second->isOnline() || !node->second->isReady()) {
                    node++;
                    continue;
                }
                TcpLink* link = node->second;
                ClientData* bundle = new ClientData();
                bundle->id = node->first;;
                try {
                    bundle->size = link->receive_data<std::size_t>();
                } catch (const BrokenPipe& broken_pipe) {
                    // conection to worker lost
                    delete node->second;
                    node->second = NULL;
                    delete bundle;
                    node++;
                    continue;
                }
                if (bundle->size == 0) {
                    // not furthur data expected
                    delete bundle;
                    node++;
                    continue;
                }
                void* buffer = NULL;
                try {
                    buffer = link->receive_ptr(bundle->size);
                } catch (const BrokenPipe& broken_pipe) {
                    // connection to worker lost
                    delete node->second;
                    node->second = NULL;
                    delete bundle;
                    node++;
                    continue;
                }
                if (buffer == NULL) {
                    // no data got
                    free(buffer);
                    delete bundle;
                    node++;
                    continue;
                }
                // re-assemble event data
                bundle->event = Event::assemble(buffer);
                free(buffer);
                if (bundle->event == NULL) {
                    // no event assembled
                    delete bundle;
                    node++;
                    continue;
                }
                // add bundle to system
                this->recv.lock();
                this->incomming.push(bundle);
                this->recv.unlock();
                // next worker
                node++;
            }
            this->workers.unlock();
            // Delay
            SDL_Delay(100);
        }
    }

    void Server::start(unsigned short port) {
        if (this->isOnline()) {
            return;
        }    
        // start listener
        this->listener.open(port);
        this->thread.start(server, this);
    }

    bool Server::isOnline() {
        return this->listener.isOnline();
    }

    void Server::shutdown() {
        if (!this->isOnline()) {
            return;
        }
        // shutdown listener
        this->listener.close();
        this->thread.stop();
        // close and delete worker links
        for (auto node = this->links.begin(); node != this->links.end(); node++) {
            delete node->second;
        }
        this->links.clear();
        this->next_id = 0;
    }

    void Server::disconnect(ClientID id) {
        this->workers.lock();
        auto node = this->links.find(id);
        if (node != this->links.end()) {
            // close and destroy link
            delete node->second;
            // remove from server
            this->links.erase(node);
        }
        this->workers.unlock();
    }

    void Server::block(const std::string& ip) {
        this->ips.lock();
        this->blocks.insert(ip);
        this->ips.unlock();
    }

    void Server::unblock(const std::string& ip) {
        this->ips.lock();
        auto node = this->blocks.find(ip);
        if (node != this->blocks.end()) {
            this->blocks.erase(node);
        }
        this->ips.unlock();
    }

    ClientData* Server::pop() {
        ClientData* tmp = NULL;
        this->recv.lock();
        if (!this->incomming.empty()) {
            tmp = this->incomming.front();
            this->incomming.pop();
        }
        this->recv.unlock();
        return tmp;
    }

}

