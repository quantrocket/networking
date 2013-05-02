/*
Copyright (c) 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers a json-based networking framework for games and other software.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "clientserver.hpp"

namespace networking {

    void server_accept(Server* server) {
        server->accept_loop();
    }

    void server_send(Server* server) {
        server->send_loop();
    }

    void server_recv(Server* server) {
        server->recv_loop();
    }

    void server_handle(Server* server) {
        server->handle_loop();
    }

    // ------------------------------------------------------------------------

    Worker::Worker(Server* server, Link* link) {
        // create worker
        server->workers_mutex.lock();
        this->id     = server->next_id++;
        this->server = server;
        this->link   = link;
        // add to server
        server->workers[this->id] = this;
        server->workers_mutex.unlock();
        // set client id to client
        json::Value welcome;
        welcome["id"] = this->id;
        std::string dump = welcome.dump();
        this->link->write(dump);
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

    Server::Server(std::int16_t max_clients)
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
            Link* next_link = this->listener.accept();
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

    /// Sending-loop
    void Server::send_loop() {
        while (this->isOnline()) {
            // Wait for Next JSON Object
            json::Value obj = this->out.pop();
            while (obj.isNull()) {
                delay(25);
                if (!this->isOnline()) {
                    // Quit Loop if Offline
                    return;
                }
                obj = this->out.pop();
            }
            // Get Worker's Link
            this->workers_mutex.lock();
            ClientID id = obj["source"].getInteger();
            auto node = this->workers.find(id);
            bool found = (node != this->workers.end());
            this->workers_mutex.unlock();
            if (found) {
                Link* link = node->second->link;
                if (link->isOnline()) {
                    // Serialize Object
                    std::string dump = obj.dump();
                    // Send to Client
                    try {
                link->write(dump);
                    } catch (const BrokenPipe& bp) {
                std::cerr << "Connection to Worker #"
                          << node->first << " was lost"
                          << std::endl;
                link->close();
                    }
                }
            } else {
                std::cerr << "Worker #" << id << " not found"
                  << std::endl;
            }
        }
    }

    /// Receiving-loop
    void Server::recv_loop() {
        while (this->isOnline()) {
            // Copy Set of Workers
            this->workers_mutex.lock();
            auto workers = this->workers;
            this->workers_mutex.unlock();
            // Iterate Through Workers
            for (auto node = workers.begin(); node != workers.end(); node++) {
                Link* link = node->second->link;
                if (link->isOnline()) {
                    while (link->isReady()) {
                std::string dump;
                // Read Dumped Event
                try {
                    dump = link->read();
                } catch (const BrokenPipe& bp) {
                    link->close();
                    std::cerr << "Connection to Worker #"
                              << node->first << " was lost"
                              << std::endl;
                    continue;
                }
                // Deserialize Event
                json::Value object;
                object.load(dump);
                // Wrap Object with ClientID as Source
                json::Value wrap;
                wrap["source"] = node->first;
                wrap["payload"] = object;
                this->in.push(wrap);
                    }
                }
            }
            delay(25);
        }
    }

    /// Handle-Loop
    void Server::handle_loop() {
        while (this->isOnline()) {
            // wait for next object
            json::Value object = this->pop();
            if (object.isNull()) {
                // Null-Object
                networking::delay(15);
            } else {
                ClientID source = object["source"].getInteger();
                json::Value payload = object["payload"];
                CommandID command_id = payload["command"].getInteger();
                // Search callback
                auto callback = this->commands.find(command_id);
                if (callback == this->commands.end()) {
                    // Undefined callback
                    std::cerr << "Undefined callback for: " << object.dump();
                } else {
                    // Exexcute callback
                    callback->second(payload, source);
                }
            }
        }
    }

    void Server::start(std::uint16_t port) {
        if (this->isOnline()) {
            return;
        }
        // start listener
        this->listener.open(port);
        this->accepter.start(server_accept, this);
        this->sender.start(server_send, this);
        this->receiver.start(server_recv, this);
        // start handler
        this->handler.start(server_handle, this);
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
        this->handler.wait();
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

    json::Value Server::pop() {
        return this->in.pop();
    }

    void Server::push(json::Value object, ClientID id) {
        json::Value wrap;
        wrap["source"] = id;
        wrap["payload"] = object;
        this->out.push(wrap);
    }

    void Server::push(json::Value object) {
        this->workers_mutex.lock();
        auto workers = this->workers;
        this->workers_mutex.unlock();
        for (auto node = workers.begin(); node != workers.end(); node++) {
            if (node->second != NULL && node->second->isOnline()) {
                this->out.push(object);
            }
        }
    }

    // ------------------------------------------------------------------------

    void client_network(Client* client) {
        client->network_loop();
    }

    void client_handle(Client* client) {
        client->handle_loop();
    }

    Client::Client() {
    }

    Client::~Client() {
        this->disconnect();
    }

    /// Sending-Receiving-loop
    void Client::network_loop() {
        while (this->isOnline()) {
            // Send All JSON Objects
            while (true) {
                json::Value object = this->out.pop();
                if (object.isNull()) {
                    // Nothing Left
                    break;
                }
                // Serialize Object
                std::string dump = object.dump();
                // Send to Server
                try {
                    this->link.write(dump);
                } catch (const BrokenPipe& bp) {
                    std::cerr << "Connection to server was lost" << std::endl;
                    this->link.close();
                    return;
                }
            }
            // Receive All JSON Objects
            while (this->link.isReady()) {
                std::string dump;
                // Read Dumped Object
                try {
                    dump = this->link.read();
                } catch (const BrokenPipe& bp) {
                    std::cerr << "Connection to server was lost" << std::endl;
                    this->link.close();
                    return;
                }
                // Deserialize JSON Object
                json::Value object;
                object.load(dump);
                this->in.push(object);
            }
            delay(25);
        }
    }

    void Client::handle_loop() {
        while (this->isOnline()) {
            // wait for next object
            json::Value object = this->pop();
            if (object.isNull()) {
                // Null-Object
                networking::delay(15);
            } else {
                json::Value payload = object["payload"];
                CommandID command_id = payload["command"].getInteger();
                // Search callback
                auto callback = this->commands.find(command_id);
                if (callback == this->commands.end()) {
                    // Undefined callback
                    std::cerr << "Undefined callback for: " << object.dump();
                } else {
                    // Exexcute callback
                    callback->second(payload);
                }
            }
        }
    }

    void Client::connect(const std::string& ip, std::uint16_t port) {
        if (this->isOnline()) {
            return;
        }
        // open connection
        this->link.open(ip, port);
        // receive client id
        std::string dump = this->link.read();
        json::Value welcome;
        welcome.load(dump);
        this->id = welcome["id"].getInteger();
        // start threads
        this->networker.start(client_network, this);
        // start handler
        this->handler.start(client_handle, this);
    }

    bool Client::isOnline() {
        return this->link.isOnline();
    }

    void Client::disconnect() {
        // close connection
        this->link.close();
        this->networker.wait();
        this->handler.wait();
        // clear queues
        this->in.clear();
        this->out.clear();
    }

    json::Value Client::pop() {
        return this->in.pop();
    }

    void Client::push(json::Value data) {
        this->out.push(data);
    }

}



