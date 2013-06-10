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

#include <net/server.hpp>

namespace net {

    Worker::Worker(Server & server, tcp::Link & link)
        : server(server)
        , link(link) {
        // create worker
        server.workers_mutex.lock();
        this->id     = server.next_id++;
        // add to server
        server.workers[this->id] = this;
        server.workers_mutex.unlock();
        // set client id to client
        json::Var welcome;
        welcome["id"] = this->id;
        std::string dump = welcome.dump();
        this->link.write(dump);
    }

    Worker::~Worker() {
        this->link.close();
    }

    void Worker::disconnect() {
        this->server.disconnect(this->id);
    }

    // ------------------------------------------------------------------------

    Server::Server(std::int16_t const max_clients)
        : CallbackManager2<CommandID, json::Var &, ClientID const>()
        , max_clients(max_clients)
        , next_id(0) {
    }

    Server::~Server() {
        if (this->listener.isOnline()) {
            this->disconnect();
        }
    }

    void Server::accept_loop() {
        while (this->isOnline()) {
            // check maximum clients
            if (this->max_clients != -1) {
                this->workers_mutex.lock();
                // avoid warning (-Wsign-compare)
                std::uint16_t tmp = (std::uint16_t)(this->max_clients);
                bool full = (this->next_id == tmp);
                this->workers_mutex.unlock();
                if (full) {
                    utils::delay(1000);
                    continue;
                }
            }
            // try fetch next connection
            tcp::Link* next_link = this->listener.accept();
            if (next_link != NULL) {
                std::string ip = next_link->getHost();
                this->ips_mutex.lock();
                bool blocked = (this->ips.find(ip) != this->ips.end());
                this->ips_mutex.unlock();
                // add worker if non-blocked ip is used
                if (blocked) {
                    delete next_link;
                    std::cerr << "Worker from " << ip << " is blocked"
                              << std::endl;
                } else {
                    new Worker(*this, *next_link);
                }
            } else {
                utils::delay(25);
            }
        }
    }

    void Server::send_loop() {
        while (this->isOnline()) {
            // Wait for Next JSON Object
            json::Var obj = this->out.pop();
            while (obj.isNull()) {
                utils::delay(25);
                if (!this->isOnline()) {
                    // Quit Loop if Offline
                    return;
                }
                obj = this->out.pop();
            }
            // Get Worker's Link
            ClientID id;
            if (!obj["source"].get(id)) {
                continue;
            }
            this->workers_mutex.lock();
            auto node = this->workers.find(id);
            bool found = (node != this->workers.end());
            this->workers_mutex.unlock();
            if (found) {
                tcp::Link& link = node->second->link;
                if (link.isOnline()) {
                    // Serialize Object
                    std::string dump = obj.dump();
                    // Send to Client
                    try {
                        link.write(dump);
                    } catch (const BrokenPipe& bp) {
                        std::cerr << "Connection to Worker #" << node->first
                                  << " was lost" << std::endl;
                        link.close();
                    }
                }
            } else {
                std::cerr << "Worker #" << id << " not found" << std::endl;
            }
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
                tcp::Link& link = node->second->link;
                if (link.isOnline()) {
                    while (link.isReady()) {
                        std::string dump;
                        // Read Dumped Event
                        try {
                            dump = link.read();
                        } catch (const BrokenPipe& bp) {
                            link.close();
                            std::cerr << "Connection to Worker #" << node->first
                                      << " was lost" << std::endl;
                            continue;
                        }
                        // Deserialize Event
                        json::Var object;
                        object.load(dump);
                        // Wrap Object with ClientID as Source
                        json::Var wrap;
                        wrap["source"] = node->first;
                        wrap["payload"] = object;
                        this->in.push(wrap);
                    }
                }
            }
            utils::delay(25);
        }
    }

    void Server::handle_loop() {
        while (this->isOnline()) {
            // wait for next object
            json::Var object = this->pop();
            if (object.isNull()) {
                // Null-Object
                utils::delay(15);
            } else {
                ClientID source;
                json::Var payload = object["payload"];
                CommandID command_id;
                if (!object["source"].get(source) ||
                    !payload["command"].get(command_id)) {
                    continue;
                }
                this->trigger(command_id, payload, source);
            }
        }
    }

    void Server::start(std::uint16_t const port) {
        if (this->isOnline()) {
            return;
        }
        // start listener
        this->listener.open(port);
        this->accepter = std::thread(&Server::accept_loop, this);
        this->sender   = std::thread(&Server::send_loop,   this);
        this->receiver = std::thread(&Server::recv_loop,   this);
        // start handler
        this->handler  = std::thread(&Server::handle_loop, this);
    }

    void Server::shutdown() {
        // wait until outgoing queue is empty
        // @note: data that is pushed while this queue is waiting might be lost
        while (this->isOnline() && !this->out.isEmpty()) {
            utils::delay(15);
        }
        this->disconnect();
    }

    void Server::disconnect() {
        // shutdown listener
        this->listener.close();
        this->accepter.join();
        this->sender.join();
        this->receiver.join();
        this->handler.join();
        // disconnect workers
        for (auto node = this->workers.begin(); node != this->workers.end();
             node++) {
            if (node->second != NULL) {
                delete node->second;
            }
        }
        this->groups.clear();
        this->workers.clear();
        this->next_id = 0;
        // clear queue
        this->out.clear();
        this->in.clear();
    }

    void Server::disconnect(ClientID const id) {
        this->workers_mutex.lock();
        auto node = this->workers.find(id);
        if (node != this->workers.end()) {
            // remove worker from groups
            auto grouplist = node->second->groups;
            for (auto n = grouplist.begin(); n != grouplist.end(); n++) {
                this->ungroup(id, *n);
            }
            // delete worker
            delete node->second;
            this->workers.erase(node);
        }
        this->workers_mutex.unlock();
    }

    void Server::push(json::Var const & object) {
        this->workers_mutex.lock();
        auto workers = this->workers;
        this->workers_mutex.unlock();
        for (auto node = workers.begin(); node != workers.end(); node++) {
            if (node->second != NULL && node->second->isOnline()) {
                json::Var wrap;
                wrap["source"] = node->first;
                wrap["payload"] = object;
                this->out.push(wrap);
            }
        }
    }

    void Server::pushGroup(json::Var const & object, GroupID const group) {
        this->groups_mutex.lock();
        auto node = this->groups.find(group);
        if (node == this->groups.end()) {
            // group does not exist
            this->groups_mutex.unlock();
            return;
        }
        // push to all group's clients
        for (auto n = node->second.begin(); n != node->second.end(); n++) {
            this->push(object, *n);
        }
        this->groups_mutex.unlock();
    }

    void Server::group(ClientID const client, GroupID const group) {
        this->groups_mutex.lock();
        auto node = this->groups.find(group);
        if (node == this->groups.end()) {
            // group does not exist, yet
            this->groups[group] = std::set<ClientID>();
            this->groups[group].insert(client);
        } else {
            // group does already exist
            node->second.insert(client);
        }
        // add this group to the clients groups
        auto n = this->workers.find(client);
        if (n != this->workers.end()) {
            n->second->groups.insert(group);
        }
        this->groups_mutex.unlock();
    }

    void Server::ungroup(ClientID const client, GroupID const group) {
        this->groups_mutex.lock();
        auto node = this->groups.find(group);
        if (node == this->groups.end()) {
            // group does not exist
            this->groups_mutex.unlock();
            return;
        }
        // remove from group
        node->second.erase(client);
        // add this group to the clients groups
        auto n = this->workers.find(client);
        if (n != this->workers.end()) {
            n->second->groups.erase(group);
        }
        this->groups_mutex.unlock();
    }

    std::set<ClientID> Server::getClients(GroupID const group) {
        this->groups_mutex.lock();
        auto node = this->groups.find(group);
        if (node == this->groups.end()) {
            // group does not exist
            this->groups_mutex.unlock();
            return std::set<ClientID>();
        }
        auto g = node->second;
        this->groups_mutex.unlock();
        // return group
        return g;
    }

    bool Server::hasGroup(GroupID const group) {
        this->groups_mutex.lock();
        auto node = this->groups.find(group);
        bool has = (node != this->groups.end());
        this->groups_mutex.unlock();
        return has;
    }

}
