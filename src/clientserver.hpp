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
#ifndef CLIENT_SERVER_INCLUDE_GUARD
#define CLIENT_SERVER_INCLUDE_GUARD

#include <set>
#include <map>
#include <cstdint>

#include "tcp.hpp"
#include "json.hpp"
#include "utils.hpp"

namespace networking {

    class Server;

    /// Type for client IDs (unsigned 16-bit integer with fixed size)
    typedef std::uint16_t ClientID;

    /// Helper function for starting the server's accepter-loop
    /**
     * This function will be started as a Thread. Is calls the server's
     *  accepter-loop to enable accepting clients in a seperate Thread.
     *  This Thread is started and stopped by the server class.
     *  @param server: pointer to the server
     */
    void server_accept(Server* server);

    /// Helper function for starting wthe server's sending-loop
    /**
     * This function will be started as a Thread. It calls the server's
     *  sending-loop to enable sending data in a seperate Thread.
     *  This Thread is started and stopped by the server class.
     *  @param server: pointer to the server
     */
    void server_send(Server* server);

    /// Helper function for starting server's receiving-loop
    /**
     * This function will be started as a Thread. It calls the server's
     *  receiving-loop to enable receiving data in a seperate Thread.
     *  This Thread is started and stopped by the server class.
     *  @param server: pointer to the server
     */
    void server_recv(Server* server);

    /// Worker
    /**
     * This is used in the context of the server class. Each client is handled
     *  by a worker. This worker contains the client ID, the TCP link and the
     *  pointer to the related server.
     */
    class Worker {
        friend class Server;
    
        protected:
            /// client ID
            ClientID id;
            /// related server
            Server* server;
            /// TCP link to the client
            Link* link;

            /// Disconnects the worker
            virtual void disconnect();

        public:
            /// Constructor
            /**
             * This creates a new worker at the related server dealing with the
             *  given TCP link. It will automatically retrieve the next valid
             *  client ID and start the necessary Threads
             */
            Worker(Server* server, Link* link);
            
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
            virtual bool isOnline();
        
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
    class Server {
        friend class Worker;
        friend void server_accept(Server* server);
        friend void server_send(Server* server);
        friend void server_recv(Server* server);
    
        protected:
            /// Listener for accepting clients
            Listener listener;
            /// Thread for accepting-loop
            Thread accepter;
            /// Thread for sending-loop
            Thread sender;
            /// Thread for receiving-loop
            Thread receiver;
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
            JsonQueue in;
            /// Queue of outgoing objects
            JsonQueue out;
            
            /// Accepter-loop
            void accept_loop();
            /// Sending-loop
            void send_loop();
            /// Receiving-loop
            void recv_loop();
        
        public:
            /// Constructor
            /**
             * Create a new server with a maximum number of clients (or -1 for
             *  an undefined maximum).
             */
            Server(std::int16_t max_clients=-1);
        
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
            void start(std::uint16_t port);
        
            /// Returns whether the server is online
            /**
             * Returns whether the server is online (= is listening) or not
             *  @return true if online
             */
            bool isOnline();
        
            /// Shutdown the server
            /**
             * This will stop listening on a local port and shutdown the
             *  accepter-loop Thread safely. It also will disconnect all
             *  workers, stop their Threads and delete them from the server
             */
            void shutdown();
        
            /// Disconnect a worker
            /**
             * This will disconnect a worker, stop it's Thread and delete it
             *  from the server.
             *  @param id: client ID of the worker
             */
            void disconnect(ClientID id);
        
            /// Block an IP-address
            /**
             * This will add the given IP-address to the blocking list
             *  @param ip: IP-address or hostname
             */
            void block(const std::string& ip);
        
            /// Unblock an IP-address
            /**
             * This will remove the given IP-address from the blocking list
             *  @param ip: IP-address or hostname
             */
            void unblock(const std::string& ip);
        
            /// Return the next bundle
            /**
             * This will return a pointer to the front bundle of the incomming
             *  queue and pop it from the queue. If the queue is empty it will
             *  return NULL. These bundles came from different workers. Use
             *  the bundle's client ID to find out it's source. Consider to
             *  delete the bundle after handling it.
             *  @return pointer to the next bundle
             */
            json::Value pop();
        
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
            void push(json::Value object, ClientID id);
    
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
            void push(json::Value object);
    };
    
    class Client;

    /// Helper function for starting client's receiving-sending-loop
    /**
     * This function will be started as a thread. It calls the client's
     *  receiving-sending-loop to enable sending data in a seperate thread.
     *  This thread is started and stopped by the client class.
     *  @param client: pointer to the client
     */
    void client_handle(Client* client);

    /// Client
    /**
     * The client provides communication with a given server. Sending and
     *  receiving data are handled in two seperate threads.
     */
    class Client {
        friend void client_handle(Client* client);

        private:
            /// Thread for sending-receiving-loop
            Thread handler;
            Thread receiver;
            /// Queues
            JsonQueue out;
            JsonQueue in;

            /// Sending-Receiving-loop
            void handle_loop();

        protected:
            /// Link to the server
            Link link;
            /// Client ID
            ClientID id;

        public:
            /// Constructor
            Client();

            /// Destructor
            /**
             * Will disconnect from the server
             */
            virtual ~Client();

            /// Establish connection to the server at the given IP and port
            /**
             * Establishs a connection to the server with the given IP-address,
             *  that is listening on the given port number. This method will
             *  receive the assigned client ID and start the threads for the
             *  sending- and receiving-loops. Consider that the server might
             *  refuse a client based on it's IP-address. So be aware that this
             *  method might throw a "BrokenPipe" execption.
             *  @param ip: IP-address or hostname of the remote server
             *  @param port: port number of the remove server
             */
            void connect(const std::string& ip, std::uint16_t port);

            /// Returns whether the client is online
            /**
             * Returns whether the client is connected to a server or not.
             *  @return true if connected
             */
            bool isOnline();

            /// Disconnect the current connection
            /**
             * This will close the current link to the server and safely stop
             *  the sending- and receiving-loop threads
             */
            void disconnect();

            /// Get the next incomming bundle
            /**
             * This returns the next incomming bundle from the front of the
             *  incomming queue and pops it from there. This might be NULL if
             *  the incomming queue is empty.
             *  @return pointer to bundle or NULL
             */
            json::Value pop();
    
            /// Push an json-object for sending
            /**
             * This will create a bundle using the current client ID, the given
             *  event-pointer and the event data's size. This size is taken
             *  from the size of the type TEvent, given with the event.
             *  Consider that you should not delete this event. It is
             *  automatically deleted after sending to the server.
             *  @param event: event-pointer
             */
            void push(json::Value data);

    };

}

#endif

