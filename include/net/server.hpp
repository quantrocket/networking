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

#include <json/json.hpp>

#include <net/link.hpp>
#include <net/common.hpp>
#include <net/callbacks.hpp>

namespace net {

    // prototyped for Worker class
    class Server;

    /// ID for logically grouped clients
    typedef std::uint32_t GroupID;

    /// Worker
    /**
     * This is used in the context of the server class. Each client is handled
     *  by a worker. This worker contains the client ID, the TCP link and a
     *  reference to the related server.
     */
    class Worker {
        friend class Server;

        protected:
            /// client ID
            ClientID id;
            /// related server
            Server& server;
            /// TCP link to the client
            tcp::Link& link;

            /// Disconnects the worker
            virtual void disconnect();

        public:
            /// Constructor
            /**
             * This creates a new worker at the related server dealing with the
             *  given TCP link. It will automatically retrieve the next valid
             *  client ID and start the necessary Threads
             */
            Worker(Server & server, tcp::Link & link);

            /// Destructor
            /**
             * This will close the link, shutdown the Threads safely and clear
             *  the outgoing queue.
             */
            virtual ~Worker();

            /// Returns whether the worker is online
            /**
             * Returns whether the worker is online or not.
             *  @return true if online
             */
            virtual inline bool isOnline() {
                return this->link.isOnline();
            }

            /// Set of groups this worker was assigned to
            std::set<GroupID> groups;

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
    class Server: public CallbackManager2<CommandID, json::Var &, ClientID const> {
        friend class Worker;

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
            std::map<ClientID, Worker*> workers;
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

            /// Logically grouped clients
            std::map<GroupID, std::set<ClientID>> groups;
            /// Mutex for groups
            std::mutex groups_mutex;

            /// Accepter-loop
            void accept_loop();
            /// Sending-loop
            void send_loop();
            /// Receiving-loop
            void recv_loop();
            /// Handle-loop
            void handle_loop();

            /// Thread for handle-loop
            std::thread handler;

        public:
            /// Constructor
            /**
             * Create a new server with a maximum number of clients (or -1 for
             *  an undefined maximum).
             */
            Server(std::int16_t const max_clients=-1);

            /// Destructor
            /**
             * This will shutdown the server.
             */
            virtual ~Server();

            /// Start the server to listen on a given port
            /**
             * Triggers the server to start listening on a given local port.
             *  It will start the accepter-loop as a Thread.
             *  @param port: local port number
             */
            void start(std::uint16_t const port);

            /// Returns whether the server is online
            /**
             * Returns whether the server is online (= is listening) or not
             *  @return true if online
             */
            inline bool isOnline() {
                return this->listener.isOnline();
            }

            /// Shutdown the server safely
            /**
             * This will wait for an empty outgoing queue and disconnect the
             *  server safely.
             */
            virtual void shutdown();

            /// Shutdown the server
            /**
             * This will stop listening on a local port and shutdown the
             *  accepter-loop Thread safely. It also will disconnect all
             *  workers, stop their Threads and delete them from the server
             */
            void disconnect();

            /// Disconnect a worker
            /**
             * This will disconnect a worker, stop it's Thread and delete it
             *  from the server.
             *  @param id: client ID of the worker
             */
            void disconnect(ClientID const id);

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
            inline void unblock(const std::string& ip) {
                this->ips_mutex.lock();
                auto node = this->ips.find(ip);
                if (node != this->ips.end()) {
                    this->ips.erase(node);
                }
                this->ips_mutex.unlock();
            }

            /// Return the next bundle
            /**
             * This will return a "bundle" containing the source client's id
             *  and the original data as object.
             *  @return next bundle
             */
            inline json::Var pop() {
                return this->in.pop();
            }

            /// Push an object to a worker
            /**
             * This will push an object to a given worker. The object is
             *  serialized and send through the socket. At the other side it
             *  can be popped to obtain the data.
             *  @param object: an object to send
             *  @param id: destination's client ID
             */
            inline void push(json::Var const & object, ClientID const id) {
                json::Var wrap;
                wrap["source"] = id;
                wrap["payload"] = object;
                this->out.push(wrap);
            }

            /// Push an object to all workers
            /**
             * This will push a data package to all clients. It works just like
             *  the common `push` method of this server class, but might reach
             *  more speed then notifying all clients by single calls.
             *  @param object: object to send
             */
            void push(json::Var const & object);

            /// Push an object to some workers
            /**
             * This will push a data package to all clients in the given group
             *  It works just like the common `push` method of this server
             *  class, but might reach more speed then notifying all clients by
             *  single calls.
             *  If the group does not exist, this method will ignore it. So
             *  check group existence with `hasGroup` if necessary.
             *  @param object: object to send
             *  @param group: group id to use for client notification
             */
            void pushGroup(json::Var const & object, GroupID const group);

            /// Add a client to a group
            /**
             * Will add the client to a group. If the group does not exist yet,
             *  it will be created.
             * @param client: id of the client
             * @param group: id of the group
             */
            void group(ClientID const client, GroupID const group);

            /// Remove a client from a group
            /**
             * Will remove a client from a group. If the client is not in the
             *  group, if the client does not exist or if the group does not
             *  exist, this method will ignore this.
             * @param client: id of the client
             * @param group: id of the group
             */
            void ungroup(ClientID const client, GroupID const group);

            /// Return all clients in this group
            /**
             * Will return a set of clients from this group. If the given group
             *  does not exist or is empty, an empty set will be returned.
             * @param group: id of the group
             * @return a set of clients
             */
            std::set<ClientID> getClients(GroupID const group);

            /// Returns wheather the group exists or not
            /**
             * Will return true if the group exists, else false.
             * @return true or false if group exists or not
             */
            bool hasGroup(GroupID const group);

    };

}

#endif
