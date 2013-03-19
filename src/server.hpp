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

#include "threading.hpp"
#include "connection.hpp"
#include "eventsystem.hpp"

namespace networking {

    typedef unsigned short ClientID;

    /// ClientData
    /** Contains a ClientID (as source or destination), an Event and the
     *  event's size in bytes
     */
    class ClientData {
        public:
            ClientID id;
            Event* event;
            std::size_t size;
    };
    
    class Server;
    
    void server(Server* server);

    /// Server
    class Server {
        friend void server(Server* server);
        protected:
            TcpListener listener;
            Thread thread;
            short max_clients;
            // worker management
            ClientID next_id;
            std::map<ClientID, TcpLink*> links;
            Mutex workers;
            // ip management
            std::set<std::string> blocks;
            Mutex ips;
            // data management
            std::queue<ClientData*> outgoing;
            Mutex send;
            std::queue<ClientData*> incomming;
            Mutex recv;
            
            void logic();
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
            ClientData* pop();
            template <typename TEvent> void push(TEvent* event, ClientID id) {
                // create bundle
                ClientData* bundle = new ClientData();
                bundle->id    = id;
                bundle->event = event;
                bundle->size  = sizeof(TEvent);
                // push to queue
                this->send.lock();
                this->outgoing.push(bundle);
                this->send.unlock();
            }
            template <typename TEvent> void push(TEvent* event) {
                this->workers.lock();
                auto node = this->links.begin();
                while (node != this->links.end()) {
                    if (node->second != NULL && node->second->isOnline()) {
                        // push to each worker
                        this->push(event, node->first);
                    }
                    node++;
                }
                this->workers.unlock();
            }
    };

}

#endif
