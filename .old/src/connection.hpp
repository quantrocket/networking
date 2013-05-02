/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <iostream>
#include <stdlib.h>

#include <SDL/SDL_net.h>

namespace networking {

    class TcpListener;

    /// An exception class used by a network connection
    class NetworkError: public std::exception {
        protected:
            std::string msg;
        public:
            NetworkError(const std::string& msg) throw() {
                this->msg = msg;
                std::cerr << "Networking error occured: "
                          << this->msg << std::endl;
            }
            virtual ~NetworkError() throw() {}
            virtual const char* what() const throw() {
                return this->msg.c_str();
            }
    };

    /// An exception class used in case of a broken connection
    class BrokenPipe: public std::exception {
        protected:
            std::string msg;
        public:
            BrokenPipe() throw() {
                this->msg = "Broken pipe";
                std::cerr << "Networking error occured: "
                          << this->msg << std::endl;
            }
            virtual ~BrokenPipe() throw() {}
            virtual const char* what() const throw() {
                return this->msg.c_str();
            }
    };

    /// A host with ip and port number
    /**
     *  This structure is used to simplfy host resolve and getting hostname or
     *  port number in an easier way.
     */
    struct Host {
        /// ip-address structure (from SDL_net)
        IPaddress* addr;
        /// Constructor resolving host and port
        /**
         * This resolves the given hostname and port number and creates the
         *  IPaddress object
         *  @param host: remote hostname
         *  @param post: remote port number
         */
        Host(const std::string& host, unsigned short port);
        /// Constructor resolving a port on localhost
        /**
         * This is used by servers to resolve a port on the local machine
         *  @param port: own port number
         */
        Host(unsigned short port);
        /// Constructor using a given IP-address
        /**
         * Creates an exact copy of the given IPaddress object.
         *  @param addr: ip address object to copy
         */
        Host(IPaddress* addr);
        /// Destructor
        ~Host();
        /// Get IP-address or hostname
        /**
         * Returns the IP-address or hostname of this host.
         *  @return IP-adress or hostname as string
         */
        std::string ip();
        /// Get port number
        /**
         * Returns the port number of this host.
         *  @param port number
         */
        unsigned short port();
    };

    /// A link class based on a TCP-socket
    /**
     * This class handles the communication using a TCP socket. An instance of
     *  this class can send and receive data with a fixed size or a pointer
     *  with a given length.
     */
    class TcpLink {
        friend class TcpListener;
        protected:
            /// socket set used for non-blocking receive
            SDLNet_SocketSet set;
            /// actual TCP-socket
            TCPsocket socket;
            /// Describes whether the link is online or not
            bool online;
        public:
            /// Constructor for an offline link
            TcpLink();
            /// Constructor for an online, given link
            /**
             * Creates the link from a given TCP-socket.
             *  @param socket: TCP-socket to use
             */
            TcpLink(TCPsocket socket);
            /// Destructor
            virtual ~TcpLink();
            /// Opens the connection to a host on a port
            /**
             * Resolves the host and establishs a connection. The local port
             *  number is assigned automatically.
             *  @param host: remote hostname
             *  @param port: remote port number
             */
            void open(const std::string& host, unsigned short port);
            /// Closes the connection
            void close();
            /// Returns whether the link is online or not
            /**
             * Returns whether the link is online or not. A previously broken
             *  pipe automatically sets a link to offline
             *  @return true if online
             */
            bool isOnline();
            /// Returns whether there is activity on the socket
            /**
             * Describes whether there is activity on the socket. This uses a
             *  socket set with this socket inside it. This method can be used
             *  to check the socket for activity to allow non-blocking receive.
             */
            bool isReady();
            /// Send a pointer with fixed size
            /**
             * This allows to send pointer data with a fixed length.
             *  If the connection is lost, this method will set the link to
             *  offline and throw a "BrokenPipe" exception.
             *  @param data: pointer to data
             *  @param len: length of data
             */
            void send_ptr(void* data, std::size_t len);
            /// Receive a pointer with fixed size
            /**
             * This allows to receive pointer data with a fixed length.
             *  If the connection is lost, this method will set the link to
             *  offline and throw a "BrokenPipe" exception. It also will return
             *  NULL in this case.
             *  Consider, that this method will block until the data can be
             *  read from the socket. Use isReady() to check for incomming
             *  data to allow non-blocking receive.
             *  @param len: length of data
             *  @return pointer to data or NULL
             */
            void* receive_ptr(std::size_t len);
            /// Send data of a specific type
            /**
             * This allows to send data of a specific type Data. If the
             *  connection is lost, this method will set the link to offline
             *  and throw a "BrokenPipe" exception.
             *  @param data: data to send
             */
            template <typename Data> void send_data(Data data) {
                if (this->socket == NULL) {
                    // tcp socket is not connected
                    this->online = false;
                    throw BrokenPipe();
                }
                std::size_t len = sizeof(Data);
                int sent = SDLNet_TCP_Send(this->socket, &data, len);
                if (sent < int(len)) {
                    // error while sending
                    this->online = false;
                    throw BrokenPipe();
                }
            }
            /// Receive data of a specific type
            /**
             *  This allows to receive data of a specific type Data. If the
             *  connection is lost, this method will set the link to offline
             *  and throw a "BrokenPipe" exception. It also will return the
             *  default value of the given type Data in this case.
             *  Consider, that this method will block until the data can be
             *  read from the socket. Use isReady() to check for incomming
             *  data to allow non-blocking receive.
             *  @return read value or the default value
             */
            template <typename Data> Data receive_data() {
                if (this->socket == NULL) {
                    // tcp socket is not connected
                    this->online = false;
                    throw BrokenPipe();
                }
                Data data;
                std::size_t len = sizeof(Data);
                int read = SDLNet_TCP_Recv(this->socket, &data, len);
                if (read <= 0) {
                    // error while receiving
                    this->online = false;
                    throw BrokenPipe();
                }
                return data;
            }
            /// Host data of the link
            Host* host;
    };

    /// A listener class for TCP links
    /**
     *  This class is listening for new TCP links.
     */
    class TcpListener {
        protected:
            /// server socket used for listening
            TCPsocket socket;
        public:
            /// Constructor
            TcpListener();
            /// Destructor
            virtual ~TcpListener();
            /// Starts listening on a local port
            /**
             * This starts the server socket on a given local port.
             *  @param port: local port number
             */
            void open(unsigned short port);
            /// Returns whether the server socket is online or not
            /**
             * Returns whether the server socke4t is online or not
             *  @return true if online
             */
            bool isOnline();
            /// Stops listening
            /**
             * This stops the server socket on the current local port.
             */
            void close();
            /// Accept an incomming connection on TCP
            /**
             * This accepts an incomming connection on TCP and returns a
             *  pointer to the TcpLink. If there is no incomming connection
             *  it will return NULL.
             *  @return pointer to TcpLink or NULL
             */
            TcpLink* accept();
    };

}

#endif
