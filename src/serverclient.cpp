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
        server->accept();
    }

    void worker_send(Worker* worker) {
        worker->send();
    }

    void worker_recv(Worker* worker) {
        worker->recv();
    }

    void client_send(Client* client) {
        client->send();
    }

    void client_recv(Client* client) {
        client->recv();
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
        this->sender.start(worker_send, this);
        this->receiver.start(worker_recv, this);
        server->workers[this->id] = this;
        server->workers_mutex.unlock();
        // send client id to client
        this->link->send_data(this->id);
    }

    Worker::~Worker() {
        this->link->close();
        this->sender.stop();
        this->receiver.stop();
        this->out.clear();
    }

    void Worker::disconnect() {
        this->server->disconnect(this->id);
    }

    void Worker::send() {
        while (this->isOnline()) {
            // pop next bundle
            Bundle* bundle = this->out.pop();
            if (bundle != NULL) {
                if (bundle->id == this->id) {
                    // send
                    try {
                        link->send_data<std::size_t>(bundle->size);
                        link->send_ptr(bundle->event, bundle->size);
                    } catch (const BrokenPipe& bp) {
                        link->close();
                        return;
                    }
                } else {
                    std::cerr << "data for #" << bundle->id
                              << " cannot be sent by this worker"
                              << std::endl;
                }
                delete bundle;
            }
            delay(25);
        }
    }

    void Worker::recv() {
        while (this->isOnline()) {
            if (this->link->isReady()) {
                // load size
                std::size_t size;
                try {
                    size = this->link->receive_data<std::size_t>();
                } catch (const BrokenPipe& bp) {
                    this->link->close();
                    return;
                }
                if (size > 0) {
                    // wait for event (break if link went offline)
                    while (!this->link->isReady()) {
                        delay(10);
                        if (!this->isOnline()) {
                            return;
                        }
                    }
                    // load event
                    void* buffer;
                    try {
                        buffer = link->receive_ptr(size);
                    } catch (const BrokenPipe& bp) {
                        this->link->close();
                        return;
                    }
                    // assemble event
                    Event* event = Event::assemble(buffer);
                    free(buffer);
                    if (event != NULL) {
                        // create bundle
                        Bundle* bundle = new Bundle(this->id, event, size);
                        // push bundle to server
                        this->server->in.push(bundle);
                    }
                }
            } else {
                delay(25);
            }
        }
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

    void Server::accept() {
        while (this->isOnline()) {
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

    void Server::start(unsigned short port) {
        if (this->isOnline()) {
            return;
        }
        // start listener
        this->listener.open(port);
        this->accepter.start(server_accept, this);
    }

    bool Server::isOnline() {
        return this->listener.isOnline();
    }

    void Server::shutdown() {
        // shutdown listener
        this->listener.close();
        this->accepter.stop();
        // disconnect workers
        for (auto node = this->workers.begin(); node != this->workers.end(); node++) {
            if (node->second != NULL) {
                delete node->second;
            }
        }
        this->workers.clear();
        this->next_id = 0;
        // clear queue
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

    void Client::send() {
        while (this->isOnline()) {
            // pop next bundle
            Bundle* bundle = this->out.pop();
            if (bundle != NULL) {
                if (bundle->id == this->id) {
                    // send
                    try {
                        link.send_data<std::size_t>(bundle->size);
                        link.send_ptr(bundle->event, bundle->size);
                    } catch (const BrokenPipe& bp) {
                        link.close();
                        return;
                    }
                } else {
                    std::cerr << "data from #" << bundle->id
                              << " cannot be sent by this client"
                              << std::endl;
                }
                delete bundle;
            }
            delay(25);
        }
    }

    void Client::recv() {
        while (this->isOnline()) {
            if (this->link.isReady()) {
                // load size
                std::size_t size;
                try {
                    size = this->link.receive_data<std::size_t>();
                } catch (const BrokenPipe& bp) {
                    this->link.close();
                    return;
                }
                if (size > 0) {
                    // wait for event (break if link went offline)
                    while (!this->link.isReady()) {
                        delay(10);
                        if (!this->isOnline()) {
                            return;
                        }
                    }
                    // load event
                    void* buffer;
                    try {
                        buffer = link.receive_ptr(size);
                    } catch (const BrokenPipe& bp){
                        this->link.close();
                        return;
                    }
                    // assemble event
                    Event* event = Event::assemble(buffer);
                    free(buffer);
                    if (event != NULL) {
                        // create bundle
                        Bundle* bundle = new Bundle(this->id, event, size);
                        // push bundle to queue
                        this->in.push(bundle);
                    }
                }
            } else {
                delay(25);
            }
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
        this->sender.start(client_send, this);
        this->receiver.start(client_recv, this);
    }

    bool Client::isOnline() {
        return this->link.isOnline();
    }

    void Client::disconnect() {
        // close connection
        this->link.close();
        this->sender.stop();
        this->receiver.stop();
        // clear queues
        this->in.clear();
        this->out.clear();
    }

    Bundle* Client::pop() {
        return this->in.pop();
    }

}

