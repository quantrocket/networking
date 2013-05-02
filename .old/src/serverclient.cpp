/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "serverclient.hpp"

namespace networking {

    void server_accept(Server* server) {
        server->accept_loop();
    }

    void server_send(Server* worker) {
        worker->send_loop();
    }

    void server_recv(Server* server) {
        server->recv_loop();
    }

    void client_handle(Client* client) {
        client->handle_loop();
    }

    // ------------------------------------------------------------------------

    Bundle::Bundle(ClientID id, Event* event, std::size_t size)
        : id(id)
        , event(event)
        , size(size) {
    }

    Bundle::~Bundle() {
        if (this->event != NULL) {
            delete this->event;
        }
    }

    // ------------------------------------------------------------------------

    Worker::Worker(Server* server, TcpLink* link) {
        // create worker and add to server
        server->workers_mutex.lock();
        this->id     = server->next_id++;
        this->server = server;
        this->link   = link;
        server->workers[this->id] = this;
        server->workers_mutex.unlock();
        // send client id to client
        this->link->send_data(this->id);
    }

    Worker::~Worker() {
        this->link->close();
    }

    void Worker::disconnect() {
        this->server->disconnect(this->id);
    }

    bool Worker::isOnline() {
        return this->link->isOnline();
    }

    // ------------------------------------------------------------------------

    Server::Server(short max_clients)
        : max_clients(max_clients)
        , next_id(0) {
    }

    Server::~Server() {
        this->shutdown();
    }

    void Server::accept_loop() {
        while (this->isOnline()) {
            // check maximum clients
            if (this->max_clients != -1) {
                this->workers_mutex.lock();
                bool full = (this->next_id == this->max_clients);
                this->workers_mutex.unlock();
                if (full) {
                    delay(1000);
                    continue;
                }
            }
            // try fetch next connection
            TcpLink* next_link = this->listener.accept();
            if (next_link != NULL) {
                std::string ip = next_link->host->ip();
                this->ips_mutex.lock();
                bool blocked = (this->ips.find(ip) != this->ips.end());
                this->ips_mutex.unlock();
                // add worker if non-blocked ip is used
                if (blocked) {
                    delete next_link;
                    std::cerr << "Worker from " << ip << " is blocked"
                              << std::endl;
                } else {
                    new Worker(this, next_link);
                }
            } else {
                delay(25);
            }
        }
    }

    void Server::send_loop() {
        while (this->isOnline()) {
            // Wait for Next Bundle
            Bundle* bundle = this->out.pop();
            while (bundle == NULL) {
                delay(25);
                if (!this->isOnline()) {
                    // Quit Loop if Offline
                    return;
                }
                bundle = this->out.pop();
            }
            // Get Worker's Link
            this->workers_mutex.lock();
            auto node = this->workers.find(bundle->id);
            bool found = (node != this->workers.end());
            this->workers_mutex.unlock();
            if (found) {
                TcpLink* link = node->second->link;
                if (link->isOnline()) {
                    // Actual Sending
                    try {
                        link->send_data<std::size_t>(bundle->size);
                        link->send_ptr(bundle->event, bundle->size);
                    } catch (const BrokenPipe& bp) {
                        std::cerr << "Connection to Worker #"
                                  << node->first << " was lost"
                                  << std::endl;
                        link->close();
                    }
                }
            } else {
                std::cerr << "Worker #" << bundle->id << " not found"
                          << std::endl;
            }
            delete bundle;
        }
    }

    void Server::recv_loop() {
        while (this->isOnline()) {
            // Copy Set of Workers
            this->workers_mutex.lock();
            auto workers = this->workers;
            this->workers_mutex.unlock();
            // Iterate Through Workers
            for (auto node = workers.begin(); node != workers.end(); node++) {
                TcpLink* link = node->second->link;
                if (link->isOnline()) {
                    while (link->isReady()) {
                        // Load Event Size
                        std::size_t size;
                        try {
                            size = link->receive_data<std::size_t>();
                        } catch (const BrokenPipe& bp) {
                            link->close();
                            std::cerr << "Connection to Worker #"
                                      << node->first << " was lost"
                                      << std::endl;
                            continue;
                        }
                        if (size == 0) {
                            std::cerr << "Worker #" << node->first
                                      << " received an empty event"
                                      << std::endl;
                            continue;
                        }
                        // Wait for Event Data
                        while (!link->isReady()) {
                            delay(10);
                            if (!link->isOnline()) {
                                // Continue if Worker Offline
                                std::cerr << "Worker #" << node->first
                                          << " is offline" << std::endl;
                                continue;
                            }
                        }
                        // Load Buffer
                        void* buffer;
                        try {
                            buffer = link->receive_ptr(size);
                        } catch (const BrokenPipe& bp) {
                            std::cerr << "Connection to Worker #"
                                      << node->first << " was lost"
                                      << std::endl;
                            link->close();
                            continue;
                        }
                        // Assemble Event Data
                        Event* event = Event::assemble(buffer);
                        free(buffer);
                        if (event != NULL) {
                            // Create Bundle and Push to Queue
                            Bundle* bundle = new Bundle(node->first,
                                event, size);
                            this->in.push(bundle);
                        } else {
                            std::cerr << "Worker #" << node->first
                                      << " received an empty event"
                                      << std::endl;
                        }
                    }
                }
            }
            delay(25);
        }
    }

    void Server::start(unsigned short port) {
        if (this->isOnline()) {
            return;
        }
        // start listener
        this->listener.open(port);
        this->accepter.start(server_accept, this);
        this->sender.start(server_send, this);
        this->receiver.start(server_recv, this);
    }

    bool Server::isOnline() {
        return this->listener.isOnline();
    }

    void Server::shutdown() {
        // shutdown listener
        this->listener.close();
        this->accepter.wait();
        this->sender.wait();
        this->receiver.wait();
        // disconnect workers
        for (auto node = this->workers.begin();
             node != this->workers.end(); node++) {
            if (node->second != NULL) {
                delete node->second;
            }
        }
        this->workers.clear();
        this->next_id = 0;
        // clear queue
        this->out.clear();
        this->in.clear();
    }

    void Server::disconnect(ClientID id) {
        this->workers_mutex.lock();
        auto node = this->workers.find(id);
        if (node != this->workers.end()) {
            // delete worker
            delete node->second;
            this->workers.erase(node);
        }
        this->workers_mutex.unlock();
    }

    void Server::block(const std::string& ip) {
        this->ips_mutex.lock();
        this->ips.insert(ip);
        this->ips_mutex.unlock();
    }

    void Server::unblock(const std::string& ip) {
        this->ips_mutex.lock();
        auto node = this->ips.find(ip);
        if (node != this->ips.end()) {
            this->ips.erase(node);
        }
        this->ips_mutex.unlock();
    }

    Bundle* Server::pop() {
        return this->in.pop();
    }

    // ------------------------------------------------------------------------

    Client::Client() {
    }

    Client::~Client() {
        this->disconnect();
    }

    void Client::handle_loop() {
        while (this->isOnline()) {
            // Send All Bundles
            while (true) {
                Bundle* bundle = this->out.pop();
                if (bundle == NULL) {
                    // Nothing Left
                    break;
                }
                // Send to Server
                try {
                    this->link.send_data<std::size_t>(bundle->size);
                    this->link.send_ptr(bundle->event, bundle->size);
                } catch (const BrokenPipe& bp) {
                    std::cerr << "Connection to server was lost" << std::endl;
                    this->link.close();
                    return;
                }
                // Delete Bundle
                delete bundle;
            }
            // Receive All Bundles
            while (this->link.isReady()) {
                // Load Size
                std::size_t size;
                try {
                    size = this->link.receive_data<std::size_t>();
                } catch (const BrokenPipe& bp) {
                    std::cerr << "Connection to server was lost" << std::endl;
                    this->link.close();
                    return;
                }
                if (size == 0) {
                    std::cerr << "Client received an empty event" << std::endl;
                    continue;
                }
                // Wait for Event Data
                while (!this->link.isReady()) {
                    delay(10);
                    if (!this->isOnline()) {
                        std::cerr << "Connection to server was lost"
                                  << std::endl;
                        return;
                    }
                }
                // Load Buffer
                void* buffer;
                try {
                    buffer = this->link.receive_ptr(size);
                } catch (const BrokenPipe& bp) {
                    std::cerr << "Connection to server was lost" << std::endl;
                    this->link.close();
                    return;
                }
                // Assemble Event Data
                Event* event = Event::assemble(buffer);
                free(buffer);
                if (event != NULL) {
                    // Create Bundle and Push to Queue
                    Bundle* bundle = new Bundle(this->id, event, size);
                    this->in.push(bundle);
                } else {
                    std::cerr << "Client received an empty event" << std::endl;
                }
            }
            delay(25);
        }
    }

    void Client::connect(const std::string& ip, unsigned short port) {
        if (this->isOnline()) {
            return;
        }
        // open connection
        this->link.open(ip, port);
        // receive client id
        this->id = this->link.receive_data<ClientID>();
        // start threads
        this->handler.start(client_handle, this);
    }

    bool Client::isOnline() {
        return this->link.isOnline();
    }

    void Client::disconnect() {
        // close connection
        this->link.close();
        this->handler.wait();
        // clear queues
        this->in.clear();
        this->out.clear();
    }

    Bundle* Client::pop() {
        return this->in.pop();
    }

}

