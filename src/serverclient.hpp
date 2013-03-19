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

    /// Type for client IDs (unsigned 16-bit integer with fixed size)
    typedef uint16_t ClientID;

    /// Helper function for starting the server's accepter-loop
    /**
     * This function will be started as a thread. Is calls the server's
     *  accepter-loop to enable accepting clients in a seperate thread.
     *  This thread is started and stopped by the server class.
     *  @param server: pointer to the server
     */
    void server_accept(Server* server);
    /// Helper function for starting worker's sending-loop
    /**
     * This function will be started as a thread. It calls the worker's
     *  sending-loop to enable sending data in a seperate thread.
     *  This thread is started and stopped by the worker class.
     *  @param worker: pointer to the worker
     */
    void worker_send(Worker* worker);
    /// Helper function for starting worker's receiving-loop
    /**
     * This function will be started as a thread. It calls the worker's
     *  receiving-loop to enable receiving data in a seperate thread.
     *  This thread is started and stopped by the worker class.
     *  @param worker: pointer to the worker
     */
    void worker_recv(Worker* worker);
    /// Helper function for starting client's sending-loop
    /**
     * This function will be started as a thread. It calls the client's
     *  sending-loop to enable sending data in a seperate thread.
     *  This thread is started and stopped by the client class.
     *  @param client: pointer to the client
     */
    void client_send(Client* client);
    /// Helper function for starting client's receiving-loop
    /**
     * This function will be started as a thread. It calls the client's
     *  receiving-loop to enable receiving data in a seperate thread.
     *  This thread is started and stopped by the client class.
     *  @param client: pointer to the client
     */
    void client_recv(Client* client);

    /// Bundle of client ID, event pointer and event size
    /**
     * This bundle of data contains the client ID, an event pointer and the
     *  event's size. The client ID is used as source for receiving or as
     *  destination for sending. The major use of this ID is at the server.
     */
    class Bundle {
        public:
            /// Constructor with id, event pointer and size
            /**
             * Creates a bundle of data from the given parameters.
             *  @param id: client ID of this bundle
             *  @param event: event-pointer
             *  @param size: size of the event-data
             */
            Bundle(ClientID id, Event* event, std::size_t size);
            /// Destructor
            /**
             * It will delete the given event-pointer.
             */
            virtual ~Bundle();
            /// client ID
            ClientID id;
            /// event-pointer
            Event* event;
            /// size of event-data
            std::size_t size;
    };

    /// Worker
    /**
     * This is used in the context of the server class. Each client is handled
     *  by a worker. This worker contains the client ID, the TCP link, the
     *  pointer to the related server and an queue for outgoing bundles for
     *  sending. It uses single threads for sending and receiving. All received
     *  bundles are pushed to the server's incomming queue.
     */
    class Worker {
        friend class Server;
        friend void worker_send(Worker* worker);
        friend void worker_recv(Worker* worker);
        protected:
            /// client ID
            ClientID id;
            /// related server
            Server* server;
            /// TCP link to the client
            TcpLink* link;
            /// outgoing bundles queue
            ThreadSafeQueue<Bundle> out;
            /// thread for sending-loop
            Thread sender;
            /// thread for receiving-loop
            Thread receiver;
            /// Disconnects the worker
            void disconnect();
            /// Sending-loop
            void send();
            /// non-blocking Receiving-loop
            void recv();
        public:
            /// Constructor
            /**
             * This creates a new worker at the related server dealing with the
             *  given TCP link. It will automatically retrieve the next valid
             *  client ID and start the necessary threads
             */
            Worker(Server* server, TcpLink* link);
            /// Destructor
            /**
             * This will close the link, shutdown the threads safely and clear
             *  the outgoing queue.
             */
            virtual ~Worker();
            /// Returns whether the worker is online
            /**
             * Returns whether the worker is online or not.
             *  @return true if online
             */
            bool isOnline();
    };

    /// Server
    /**
     * The server handles all clients using threads. It can handle clients
     *  until a given, fixed maximum or more, if a maximum of -1 is given.
     *  Also it provides banning and unbanning IPs to refuse clients. The
     *  server contains an incomming queue with received bundles for all
     *  workers.
     */
    class Server {
        friend class Worker;
        friend void server_accept(Server* server);
        protected:
            /// Listener for accepting clients
            TcpListener listener;
            /// Thread for accepting-loop
            Thread accepter;
            /// Maximum number of clients (-1 = infinite)
            short max_clients;
            /// Next worker's ID
            ClientID next_id;
            /// Structure of all workers keyed by their IDs
            std::map<ClientID, Worker*> workers;
            /// Mutex for next_id and workers
            Mutex workers_mutex;
            /// Set of blocked IPs
            std::set<std::string> ips;
            /// Mutex for ips
            Mutex ips_mutex;
            /// Queue of incomming bundles
            ThreadSafeQueue<Bundle> in;
            /// Accepter-loop
            void accept();
        public:
            /// Constructor
            /**
             * Create a new server with a maximum number of clients (or -1 for
             *  an undefined maximum).
             */
            Server(short max_clients=-1);
            /// Destructor
            /**
             * This will shutdown the server.
             */
            virtual ~Server();
            /// Start the server to listen on a given port
            /**
             * Triggers the server to start listening on a given local port.
             *  It will start the accepter-loop as a thread.
             *  @param port: local port number
             */
            void start(unsigned short port);
            /// Returns whether the server is online
            /**
             * Returns whether the server is online (= is listening) or not
             *  @return true if online
             */
            bool isOnline();
            /// Shutdown the server
            /**
             * This will stop listening on a local port and shutdown the
             *  accepter-loop thread safely. It also will disconnect all
             *  workers, stop their threads and delete them from the server
             */
            void shutdown();
            /// Disconnect a worker
            /**
             * This will disconnect a worker, stop it's thread and delete it
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
            Bundle* pop();
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

    /// Client
    /**
     * The client provides communication with a given server. Sending and
     *  receiving data are handled in two seperate threads. Each event is
     *  wrapped by a bundle, which contains the destination client ID and
     *  the event data' size. These data are not necessary for the client's
     *  basic work.
     */
    class Client {
        friend void client_send(Client* client);
        friend void client_recv(Client* client);
        private:
            /// Thread for sending-loop
            Thread sender;
            /// Thread for receiving-loop
            Thread receiver;
            /// Queue for outgoing bundles
            ThreadSafeQueue<Bundle> out;
            /// Queue for incomming bundles
            ThreadSafeQueue<Bundle> in;
            /// Sending-loop
            void send();
            /// Receiving-loop
            void recv();
        protected:
            /// Tcp-Link to the server
            TcpLink link;
            /// Client ID (assigned by server)
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
            void connect(const std::string& ip, unsigned short port);
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
            Bundle* pop();
            /// Push an event for sending
            /**
             * This will create a bundle using the current client ID, the given
             *  event-pointer and the event data's size. This size is taken
             *  from the size of the type TEvent, given with the event.
             *  Consider that you should not delete this event. It is
             *  automatically deleted after sending to the server.
             *  @param event: event-pointer
             */
            template <typename TEvent> void push(TEvent* event) {
                // create bundle
                Bundle* bundle = new Bundle(this->id, event, sizeof(TEvent));
                // push to queue
                this->out.push(bundle);
            }
    };

}

#endif
