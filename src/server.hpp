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

int server(void* param);

/// Server
/**
 *  The class provides primitive non-threaded Server-Client communication.
 *  Each client is handled by a worker using an id, the tcp link and the
 *  networking queue.
 *
 *  To trigger accepting new clients you can use
 *      virtual void logic()
 *  To trigger a worker's logic you can use
 *      virtual void logic(Worker* worker)
 *
 *  Remember to add mutex/locking then using threads. The call of connect()
 *  and disconnect() should be thread-safe referring to the call of a worker's
 *  logic.
 *
 *  If you want to iterate through all workers to call their' logics using a
 *  thread, you should iterate through a copy of the workers-map, because
 *  disconnect() will remove the worker from the map. So you will crash using
 *  an iterator on the original map because your current worker-node
 *  will be invalid after disconnect()'ing him. You will not be able to iterate
 *  to the next worker properly. A copy of the map would avoid this, because
 *  you will iterate through a previously made copy. So the original map can be
 *  manipulated without effecting your current iteration.
 *  This might also be relevant for seperat accepting-clients-threads and the
 *  call of connect(), which will create a worker and append him to the map.
 */
class Server {
    friend int server(void* param);
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
        void shutdown(bool immediately=false);
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


#endif
