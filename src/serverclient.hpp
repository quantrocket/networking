/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef SERVER_HPP
#define SERVER_HPP

#include <queue>
#include <set>
#include <map>
#include <cstdint>

#include "threading.hpp"
#include "connection.hpp"
#include "eventsystem.hpp"

namespace networking {

    class Bundle;
    class Worker;
    class Server;
    class Client;

    typedef uint16_t ClientID;

    void server_accept(Server* server);
    void worker_send(Worker* worker);
    void worker_recv(Worker* worker);

    void client_send(Client* client);
    void client_recv(Client* client);

    // ------------------------------------------------------------------------

    class Bundle {
        public:
            Bundle(ClientID id, Event* event, std::size_t size);
            virtual ~Bundle();

            ClientID id;
            Event* event;
            std::size_t size;
    };

    // ------------------------------------------------------------------------

    /// Worker
    class Worker {
        friend class Server;
        friend void worker_send(Worker* worker);
        friend void worker_recv(Worker* worker);
        protected:
            ClientID id;
            Server* server;
            TcpLink* link;
            ThreadSafeQueue<Bundle> out;
            Thread sender;
            Thread receiver;

            void disconnect();
            void send();
            void recv();
        public:
            Worker(Server* server, TcpLink* link);
            virtual ~Worker();
            bool isOnline();
    };

    // ------------------------------------------------------------------------

    /// Server
    class Server {
        friend class Worker;
        friend void server_accept(Server* server);
        protected:
            TcpListener listener;
            Thread accepter;
            short max_clients;
            // worker management
            ClientID next_id;
            std::map<ClientID, Worker*> workers;
            Mutex workers_mutex;
            // ip management
            std::set<std::string> ips;
            Mutex ips_mutex;
            // data management
            ThreadSafeQueue<Bundle> in;

            void accept();
            void work(Worker* worker);
        public:
            // server management
            Server(short max_clients=-1);
            virtual ~Server();
            void start(unsigned short port);
            bool isOnline();
            void shutdown();
            // worker management
            void disconnect(ClientID id);
            // ip management
            void block(const std::string& ip);
            void unblock(const std::string& ip);
            // data management
            Bundle* pop();

            template <typename TEvent> void push(TEvent* event, ClientID id) {
                this->workers_mutex.lock();
                // Search Worker
                auto node = this->workers.find(id);
                if (node != this->workers.end()) {
                    Worker* worker = node->second;
                    this->workers_mutex.unlock();
                    // create bundle
                    Bundle* bundle = new Bundle(id, event, sizeof(TEvent));
                    // push to his queue
                    worker->out.push(bundle);
                } else {
                    this->workers_mutex.unlock();
                    std::cerr << "No worker #" << id << " found" << std::endl;
                }
            }

            template <typename TEvent> void push(TEvent* event) {
                this->workers_mutex.lock();
                auto node = this->workers.begin();
                while (node != this->workers.end()) {
                    if (node->second != NULL && node->second->isOnline()) {
                        Worker* worker = node->second;
                        // create bundle with copy of event
                        Bundle* bundle = new Bundle(worker->id, new TEvent(event), sizeof(TEvent));
                        // push to each worker
                        worker->out.push(bundle);
                    }
                    node++;
                }
                // event was copied, so can be deleted
                delete event;
                this->workers_mutex.unlock();
            }
    };

    // ------------------------------------------------------------------------

    /// Client
    class Client {
        friend void client_send(Client* client);
        friend void client_recv(Client* client);
        private:
            Thread sender;
            Thread receiver;
            // data management
            ThreadSafeQueue<Bundle> out;
            ThreadSafeQueue<Bundle> in;

            void send();
            void recv();
        protected:
            TcpLink link;
            ClientID id;
        public:
            // connection management
            Client();
            virtual ~Client();
            void connect(const std::string& ip, unsigned short port);
            bool isOnline();
            void disconnect();
            // data management
            Bundle* pop();

            template <typename TEvent> void push(TEvent* event) {
                // create bundle
                Bundle* bundle = new Bundle(this->id, event, sizeof(TEvent));
                // push to queue
                this->out.push(bundle);
            }
    };

}

#endif
