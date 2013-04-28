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

#pragma once
#ifndef NET_SERVER_INCLUDE_GUARD
#define NET_SERVER_INCLUDE_GUARD

#include <set>
#include <map>
#include <cstdint>

#include "link.hpp"
#include "json.hpp"
#include "common.hpp"

namespace net {

    template <typename Derived> class Server;

    /// Worker
    /**
     * This is used in the context of the server class. Each client is handled
     *  by a worker. This worker contains the client ID, the TCP link and the
     *  pointer to the related server.
     */
    template <typename Derived>
    class Worker {
        friend class Server<Derived>;

        protected:
            /// client ID
            ClientID id;
            /// related server
            Derived& server;
            /// TCP link to the client
            tcp::Link& link;

            /// Disconnects the worker
            virtual inline void disconnect() {
                this->server.disconnect(this->id);
            }

        public:
            /// Constructor
            /**
             * This creates a new worker at the related server dealing with the
             *  given TCP link. It will automatically retrieve the next valid
             *  client ID and start the necessary Threads
             */
            Worker(Derived & server, tcp::Link & link)
                : server(server)
                , link(link) {
                // create worker
                server.workers_mutex.lock();
                this->id     = server.next_id++;
                /*
                this->server = server;
                this->link   = link;
                */
                // add to server
                server.workers[this->id] = this;
                server.workers_mutex.unlock();
                // set client id to client
                json::Var welcome;
                welcome["id"] = this->id;
                std::string dump = welcome.dump();
                this->link.write(dump);
            }

            /// Destructor
            /**
             * This will close the link, shutdown the Threads safely and clear
             *  the outgoing queue.
             */
            virtual ~Worker() {
                this->link.close();
            }

            /// Returns whether the worker is online
            /**
             * Returns whether the worker is online or not.
             *  @return true if online
             */
            virtual inline bool isOnline() {
                return this->link.isOnline();
            }

    };

    /// Server
    /**
     * The server handles all clients using Threads. It can handle clients
     *  until a given, fixed maximum or more, if a maximum of -1 is given.
     *  Also it provides banning and unbanning IPs to refuse clients. The
     *  server contains an incomming queue with received bundles for all
     *  workers and an outgoing queue with bundles ready for sending to a
     *  worker.
     */
    template <typename Derived>
    class Server {
        friend class Worker<Derived>;

        protected:
            /// Listener for accepting clients
            tcp::Listener listener;
            /// Thread for accepting-loop
            std::thread accepter;
            /// Thread for sending-loop
            std::thread sender;
            /// Thread for receiving-loop
            std::thread receiver;
            /// Maximum number of clients (-1 = infinite)
            std::int16_t max_clients;
            /// Next worker's ID
            ClientID next_id;
            /// Structure of all workers keyed by their IDs
            std::map<ClientID, Worker<Derived>*> workers;
            /// Mutex for next_id and workers
            std::mutex workers_mutex;
            /// Set of blocked IPs
            std::set<std::string> ips;
            /// Mutex for ips
            std::mutex ips_mutex;
            /// Queue of incomming objects
            utils::SyncQueue<json::Var> in;
            /// Queue of outgoing objects
            utils::SyncQueue<json::Var> out;

            /// Command-Callback Mapper
            std::map<CommandID, void (Derived::*)(json::Var &, ClientID const)> callbacks;

            /// Fallback Handle for undefined commands
            virtual void fallback(json::Var& data, ClientID const id) = 0;

            /// Accepter-loop
            void accept_loop() {
                while (this->isOnline()) {
                    // check maximum clients
                    if (this->max_clients != -1) {
                        this->workers_mutex.lock();
                        bool full = (this->next_id == this->max_clients);
                        this->workers_mutex.unlock();
                        if (full) {
                            utils::delay(1000);
                            continue;
                        }
                    }
                    // try fetch next connection
                    tcp::Link* next_link = this->listener.accept();
                    if (next_link != NULL) {
                        std::string ip = next_link->host.ip();
                        this->ips_mutex.lock();
                        bool blocked = (this->ips.find(ip) != this->ips.end());
                        this->ips_mutex.unlock();
                        // add worker if non-blocked ip is used
                        if (blocked) {
                            delete next_link;
                            std::cerr << "Worker from " << ip << " is blocked"
                              << std::endl;
                        } else {
                            Derived& obj = static_cast<Derived&>(*this);
                            new Worker<Derived>(obj, *next_link);
                        }
                    } else {
                        utils::delay(25);
                    }
                }
            }

            /// Sending-loop
            void send_loop() {
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
                    this->workers_mutex.lock();
                    ClientID id = obj["source"].getInteger();
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
                                std::cerr << "Connection to Worker #"
                                          << node->first << " was lost"
                                          << std::endl;
                                link.close();
                            }
                        }
                    } else {
                        std::cerr << "Worker #" << id << " not found"
                                  << std::endl;
                    }
                }
            }

            /// Receiving-loop
            void recv_loop() {
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
                                    std::cerr << "Connection to Worker #"
                                              << node->first << " was lost"
                                              << std::endl;
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

            /// Handle-loop
            void handle_loop() {
                while (this->isOnline()) {
                    // wait for next object
                    json::Var object = this->pop();
                    if (object.isNull()) {
                        // Null-Object
                        utils::delay(15);
                    } else {
                        ClientID source = object["source"].getInteger();
                        json::Var payload = object["payload"];
                        CommandID command_id = payload["command"].getInteger();
                        // Search callback
                        auto entry = this->callbacks.find(command_id);
                        if (entry == this->callbacks.end()) {
                            // Use fallback handle
                            this->fallback(payload, source);
                        } else {
                            // Exexcute callback
                            auto callback = entry->second;
                            Derived* ptr = static_cast<Derived*>(this);
                            (ptr->*callback)(payload, source);
                        }
                    }
                }
            }

            /// Thread for handle-loop
            std::thread handler;

        public:
            /// Constructor
            /**
             * Create a new server with a maximum number of clients (or -1 for
             *  an undefined maximum).
             */
            Server(std::int16_t const max_clients=-1)
                : max_clients(max_clients)
                , next_id(0) {
            }

            /// Destructor
            /**
             * This will shutdown the server.
             */
            virtual ~Server() {
                this->shutdown();
            }

            /// Start the server to listen on a given port
            /**
             * Triggers the server to start listening on a given local port.
             *  It will start the accepter-loop as a Thread.
             *  @param port: local port number
             */
            void start(std::uint16_t const port) {
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

            /// Returns whether the server is online
            /**
             * Returns whether the server is online (= is listening) or not
             *  @return true if online
             */
            inline bool isOnline() {
                return this->listener.isOnline();
            }

            /// Shutdown the server
            /**
             * This will stop listening on a local port and shutdown the
             *  accepter-loop Thread safely. It also will disconnect all
             *  workers, stop their Threads and delete them from the server
             */
            void shutdown() {
                // shutdown listener
                this->listener.close();
                this->accepter.join();
                this->sender.join();
                this->receiver.join();
                this->handler.join();
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

            /// Disconnect a worker
            /**
             * This will disconnect a worker, stop it's Thread and delete it
             *  from the server.
             *  @param id: client ID of the worker
             */
            void disconnect(ClientID const id) {
                this->workers_mutex.lock();
                auto node = this->workers.find(id);
                if (node != this->workers.end()) {
                    // delete worker
                    delete node->second;
                    this->workers.erase(node);
                }
                this->workers_mutex.unlock();
            }

            /// Block an IP-address
            /**
             * This will add the given IP-address to the blocking list
             *  @param ip: IP-address or hostname
             */
            inline void block(std::string const & ip) {
                this->ips_mutex.lock();
                this->ips.insert(ip);
                this->ips_mutex.unlock();
            }

            /// Unblock an IP-address
            /**
             * This will remove the given IP-address from the blocking list
             *  @param ip: IP-address or hostname
             */
            void unblock(const std::string& ip) {
                this->ips_mutex.lock();
                auto node = this->ips.find(ip);
                if (node != this->ips.end()) {
                    this->ips.erase(node);
                }
                this->ips_mutex.unlock();
            }

            /// Return the next bundle
            /**
             * This will return a pointer to the front bundle of the incomming
             *  queue and pop it from the queue. If the queue is empty it will
             *  return NULL. These bundles came from different workers. Use
             *  the bundle's client ID to find out it's source. Consider to
             *  delete the bundle after handling it.
             *  @return pointer to the next bundle
             */
            inline json::Var pop() {
                return this->in.pop();
            }

            /// Push an event to a worker
            /**
             * This will create a bundle using the destination id, the event-
             *  pointer and the event data's size. This size is taken from
             *  the type TEvent of the event-pointer. The created bundle will
             *  be pushed to the related worker's outgoing queue. Consider
             *  that you should not delete the event after pushing it. This
             *  is done automatically after sending it.
             *  @param event: event-pointer
             *  @param id: destination's client ID
             */
            inline void push(json::Var const & object, ClientID const id) {
                json::Var wrap;
                wrap["source"] = id;
                wrap["payload"] = object;
                this->out.push(wrap);
            }

            /**
             * This will create new bundles using the event-pointer and the
             *  event data's size. This size is taken from the type TEvent of
             *  the event-pointer. The destination's client ID will be taken
             *  from the server's worker-map. Consider that you should not
             *  delete the event after pushing it. The event is multiple copied
             *  (for each worker). The original event is deleted at the end
             *  of this method. The copies are automatically after sending.
             *  @param event: event-pointer
             */
            void push(json::Var const & object) {
                this->workers_mutex.lock();
                auto workers = this->workers;
                this->workers_mutex.unlock();
                for (auto node = workers.begin(); node != workers.end(); node++) {
                    if (node->second != NULL && node->second->isOnline()) {
                        this->out.push(object);
                    }
                }
            }
    };

}

#endif
