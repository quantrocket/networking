/*
Copyright (c) 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers a tcp-based server-client framework for games and other software.

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

#include <SFML/Network.hpp>

#include <net/common.hpp>
#include <net/callbacks.hpp>

namespace net {

    // prototyped for Worker class
    template <typename Protocol>
    class Server;

    /// ID for logically grouped clients
    typedef std::uint32_t GroupID;

    /// Worker
    /**
     * This is used in the context of the server class. Each client is handled
     *  by a worker. This worker contains the client ID, the TCP link and a
     *  reference to the related server.
     */
    template <typename Protocol>
    class Worker {
        friend class Server<Protocol>;

        protected:
            /// client ID
            ClientID id;
            /// related server
            Server<Protocol>& server;
            /// TCP link to the client
            sf::TcpSocket link;

            /// Disconnects the worker
            virtual void disconnect();

        public:
            /// Constructor
            Worker(Server<Protocol> & server);

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
                return (this->link.getRemoteAddress() != sf::IpAddress::None);
            }

            /// Set of groups this worker was assigned to
            std::set<GroupID> groups;

    };


    /// Server
    /**
     * The server handles all clients using Threads. It can handle clients
     *  until a given, fixed maximum or more, if a maximum of -1 is given.
     *  Also it provides banning and unbanning IPs to refuse clients. The
     *  server contains an incomming queue with received data for all workers
     *  and an outgoing queue with data ready for sending to a worker.
     */
    template <typename Protocol>
    class Server: public CallbackManager<CommandID, Protocol &> {
        friend class Worker<Protocol>;

        protected:
            /// Listener for accepting clients
            sf::TcpListener listener;
            /// Threads
            std::thread accepter;
            std::thread networker;
            std::thread handler;
            /// Maximum number of clients (-1 = infinite)
            std::int16_t max_clients;
            /// Next worker's ID
            ClientID next_id;
            /// Structure of all workers keyed by their IDs
            std::map<ClientID, Worker<Protocol>*> workers;
            /// Set of blocked IPs
            std::set<std::string> ips;
            /// Logically grouped clients
            std::map<GroupID, std::set<ClientID>> groups;
            /// Serveral mutex stuff
            std::mutex workers_mutex;
            std::mutex ips_mutex;
            std::mutex groups_mutex;
            /// Queues
            utils::SyncQueue<Protocol> in;
            utils::SyncQueue<Protocol> out;

            /// Send / Receive next Data
            bool sendNext();
            bool receiveNext(ClientID const clientid, sf::TcpSocket & link);

            /// Threaded Loops
            void accept_loop();
            void network_loop();
            void handle_loop();

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
            bool start(std::uint16_t const port);

            /// Returns whether the server is online
            /**
             * Returns whether the server is online (= is listening) or not
             *  @return true if online
             */
            inline bool isOnline() {
                return (this->listener.getLocalPort() != 0);
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

            /// Push an object to a worker
            /**
             * This will push an object to a given worker. Then the object is
             *  sent through the socket. At the other side it can be popped to
             *  obtain the data.
             *  @param object: an object to send
             *  @param id: destination's client ID
             */
            inline void push(Protocol & object, ClientID const id) {
                // Set target client
                object.client = id;
                // Push to outgoing queue
                this->out.push(object);
            }

            /// Push an object to all workers
            /**
             * This will push a data package to all clients. It works just like
             *  the common `push` method of this server class, but might reach
             *  more speed then notifying all clients by single calls.
             *  @param object: object to send
             */
            void push(Protocol & object);

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
            void pushGroup(Protocol & object, GroupID const group);

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

    template <typename Protocol>
    Worker<Protocol>::Worker(Server<Protocol> & server)
        : server(server) {
    }

    template <typename Protocol>
    Worker<Protocol>::~Worker() {
        this->link.disconnect();
    }

    template <typename Protocol>
    void Worker<Protocol>::disconnect() {
        this->server.disconnect(this->id);
    }

    // ------------------------------------------------------------------------

    template <typename Protocol>
    Server<Protocol>::Server(std::int16_t const max_clients)
        : CallbackManager<CommandID, Protocol &>()
        , max_clients(max_clients)
        , next_id(0) {
        // Workaround for Tcp Socket Crash
        utils::SocketCrashWorkaround();
    }

    template <typename Protocol>
    Server<Protocol>::~Server() {
        if (this->isOnline()) {
            this->disconnect();
        }
    }

    template <typename Protocol>
    void Server<Protocol>::accept_loop() {
        while (this->isOnline()) { 
            // Check for next Client
            auto next = new Worker<Protocol>(*this);
            auto status = this->listener.accept(next->link);
            if (status != sf::Socket::Done) {
                // Nothing happened
                utils::delay(25);
                delete next;
                continue;
            }
            next->link.setBlocking(false);
            
            // Check number of clients
            this->workers_mutex.lock();
            auto number = this->workers.size();
            this->workers_mutex.unlock();
            std::string hostname = next->link.getRemoteAddress().toString();
            std::uint16_t port = next->link.getRemotePort();
            if (this->max_clients != -1 && number >= this->max_clients) {
                // Server is full
                next->link.disconnect();
                delete next;
                std::cerr << "New client from " << hostname << ":" << port
                          << " was refused, because the maximum limit of"
                          << " clients has been reached" << std::endl
                          << std::flush;
                utils::delay(50);
                continue;
            }
            
            // Check if blocked
            this->ips_mutex.lock();
            bool blocked = (this->ips.find(hostname) != this->ips.end());
            this->ips_mutex.unlock();
            if (blocked) {
                // This host is banned
                next->link.disconnect();
                delete next;
                std::cerr << "Client from banned host " << hostname << ":"
                          << port << " was refused" << std::endl << std::flush;
                continue;
            }

            // Assign ClientID
            this->workers_mutex.lock();
            ClientID id = this->next_id;
            sf::Packet packet;
            packet << id;
            status = next->link.send(packet);
            if (status == sf::Socket::Done) {
                // Add to Server
                next->id = id;
                this->workers[id] = next;
                this->next_id++;
                std::cerr << "Client #" << id << " accepted from " << hostname
                          << ":" << port << std::endl << std::flush;
            } else {
                next->link.disconnect();
                delete next;
            }
            this->workers_mutex.unlock();
        }
    }

    template <typename Protocol>
    bool Server<Protocol>::sendNext() {
        // Pick next from outgoing queue
        Protocol object;
        if (!this->out.pop(object)) {
            return false;
        }
        // Get Worker's Link
        this->workers_mutex.lock();
        auto node = this->workers.find(object.client);
        bool found = (node != this->workers.end());
        this->workers_mutex.unlock();
        if (!found || node->second == NULL) {
            std::cerr << "Worker #" << object.client << " was not found"
                      << std::endl << std::flush;
            return false;
        }
        sf::TcpSocket & link = node->second->link;
        
        // Send to Client
        if (!object.send(link)) {
            if (!node->second->isOnline()) {
                // Pipe broken
                link.disconnect();
                this->workers_mutex.lock();
                delete node->second;
                this->workers.erase(node);
                std::cerr << "Connection to the client #" << object.client
                          << " was killed" << std::endl << std::flush;
                this->workers_mutex.unlock();
            }
            return false;
        }
        return true;
    }

    template <typename Protocol>
    bool Server<Protocol>::receiveNext(ClientID const clientid,
                                        sf::TcpSocket & link) {
        // Try to receive next
        Protocol object;
        if (!object.receive(link)) {
            // Cannot receive
            if (!isOnline()) {
                // Pipe broken
                link.disconnect();
                this->workers_mutex.lock();
                auto node = this->workers.find(clientid);
                if (node != this->workers.end()) {
                    delete node->second;
                    this->workers.erase(node);
                    std::cerr << "Connection to the client #" << clientid
                              << " was killed" << std::endl << std::flush;
                }
                this->workers_mutex.unlock();
            }
            return false;
        }
        // Set Source ClientID
        object.client = clientid;
        // Push to incomming queue
        this->in.push(object);
        return true;
    }

    template <typename Protocol>
    void Server<Protocol>::network_loop() {
        do {
            // Send all objects
            while (this->sendNext());
            // Receive from all workers
            this->workers_mutex.lock();
            auto workers = this->workers;
            this->workers_mutex.unlock();
            for (auto node = workers.begin(); node != workers.end(); node++) {
                while (this->receiveNext(node->first, node->second->link)) {}
            }
            // delay a bit
            utils::delay(25);
        } while (this->isOnline());
    }

    template <typename Protocol>
    void Server<Protocol>::handle_loop() {
        do {
            // Pick next from incomming queue
            Protocol object;            
            if (!this->in.pop(object)) {
                // Nothing to do
                utils::delay(15);
                continue;
            }
            // Trigger callback method
            this->trigger(object.command, object);
        } while (this->isOnline());
    }

    template <typename Protocol>
    bool Server<Protocol>::start(std::uint16_t const port) {
        if (this->isOnline()) {
            // already listening
            return true;
        }
        // Start listener
        auto status = this->listener.listen(port);
        if (status != sf::Socket::Done) {
            return false;
        }
        this->listener.setBlocking(false);
        // Start threads
        this->accepter = std::thread(&Server<Protocol>::accept_loop, this);
        this->networker = std::thread(&Server<Protocol>::network_loop, this);
        this->handler  = std::thread(&Server<Protocol>::handle_loop, this);
        
        return true;
    }

    template <typename Protocol>
    void Server<Protocol>::shutdown() {
        // wait until outgoing queue is empty
        // @note: data that is pushed while this queue is waiting might be lost
        while (this->isOnline() && !this->out.isEmpty()) {
            utils::delay(15);
        }
        this->disconnect();
    }

    template <typename Protocol>
    void Server<Protocol>::disconnect() {
        // shutdown listener
        this->listener.close();
        // shutdown threads (try-catched, they might have been stopped, yet)
        try {
            this->accepter.join();
        } catch (std::system_error const & se) {}
        try {
            this->networker.join();
        } catch (std::system_error const & se) {}
        try {
            this->handler.join();
        } catch (std::system_error const & se) {}
        this->workers_mutex.lock();
        // disconnect workers
        for (auto node = this->workers.begin(); node != this->workers.end();
             node++) {
            if (node->second != NULL) {
                delete node->second;
                node->second = NULL;
            }
        }
        this->workers_mutex.unlock();
        this->groups.clear();
        this->workers.clear();
        this->next_id = 0;
        // clear queue
        this->out.clear();
        this->in.clear();
    }

    template <typename Protocol>
    void Server<Protocol>::disconnect(ClientID const id) {
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

    template <typename Protocol>
    void Server<Protocol>::push(Protocol & object) {
        this->workers_mutex.lock();
        auto workers = this->workers;
        this->workers_mutex.unlock();
        for (auto node = workers.begin(); node != workers.end(); node++) {
            if (node->second != NULL && node->second->isOnline()) {
                // Set Target ClientID
                object.client = node->first;
                this->out.push(object);
            }
        }
    }

    template <typename Protocol>
    void Server<Protocol>::pushGroup(Protocol & object, GroupID const group) {
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

    template <typename Protocol>
    void Server<Protocol>::group(ClientID const client, GroupID const group) {
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

    template <typename Protocol>
    void Server<Protocol>::ungroup(ClientID const client,
                                    GroupID const group) {
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

    template <typename Protocol>
    std::set<ClientID> Server<Protocol>::getClients(GroupID const group) {
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

    template <typename Protocol>
    bool Server<Protocol>::hasGroup(GroupID const group) {
        this->groups_mutex.lock();
        auto node = this->groups.find(group);
        bool has = (node != this->groups.end());
        this->groups_mutex.unlock();
        return has;
    }

}

#endif
