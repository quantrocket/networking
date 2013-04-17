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
#ifndef TCP_INCLUDE_GUARD
#define TCP_INCLUDE_GUARD

#include <iostream>
#include <stdlib.h>
#include <cstdint>

#include <SDL/SDL_net.h>

namespace networking {

    class Listener;

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
        Host(const std::string& host, std::uint16_t port)
            : addr(new IPaddress()) {
            if (SDLNet_ResolveHost(this->addr, host.c_str(), port) == -1) {
                throw NetworkError("SDLNet_ResolveHost: "
                                   + std::string(SDLNet_GetError()));
            }
        }

        /// Constructor resolving a port on localhost
        /**
         * This is used by servers to resolve a port on the local machine
         *  @param port: own port number
         */
        Host(std::uint16_t port)
            : addr(new IPaddress()) {
            if (SDLNet_ResolveHost(this->addr, NULL, port) == -1) {
                throw NetworkError("SDLNet_ResolveHost: "
                                   + std::string(SDLNet_GetError()));
            }
        }

        /// Constructor using a given IP-address
        /**
         * Creates an exact copy of the given IPaddress object.
         *  @param addr: ip address object to copy
         */
        Host(IPaddress* addr)
            : addr(new IPaddress()) {
            this->addr->host = addr->host;
            this->addr->port = addr->port;
        }

        /// Destructor
        ~Host() {
            if (this->addr != NULL) {
                delete this->addr;
            }
        }

        /// Get IP-address or hostname
        /**
         * Returns the IP-address or hostname of this host.
         *  @return IP-adress or hostname as string
         */
        inline std::string ip() {
            Uint32 number = SDLNet_Read32(&(this->addr->host));
            int a, b, c, d;
            a = (number & 0xFF000000) >> 24;
            b = (number & 0x00FF0000) >> 16;
            c = (number & 0x0000FF00) >> 8;
            d = (number & 0x000000FF);
            return std::to_string(a) + "." + std::to_string(b) + "."
                    + std::to_string(c) + "." + std::to_string(d);
        }

        /// Get port number
        /**
         * Returns the port number of this host.
         *  @param port number
         */
        inline std::uint16_t port() {
            return SDLNet_Read16(&(this->addr->port));
        }

    };

    /// A link class based on a TCP-socket
    /**
     * This class handles the communication using a TCP socket. An instance of
     *  this class can send and receive data with a fixed size or a pointer
     *  with a given length.
     */
    class Link {
        friend class Listener;

        protected:
            /// socket set used for non-blocking receive
            SDLNet_SocketSet set;
            /// actual TCP-socket
            TCPsocket socket;
            /// Describes whether the link is online or not
            bool online;

        public:
            /// Constructor for an offline link
            Link()
                : set(SDLNet_AllocSocketSet(1))
                , socket(NULL)
                , online(false)
                , host(NULL) {
            }

            /// Constructor for an online, given link
            /**
             * Creates the link from a given TCP-socket.
             *  @param socket: TCP-socket to use
             */
            Link(TCPsocket socket)
                : set(SDLNet_AllocSocketSet(1))
                , socket(socket)
                , online(true)
                , host(NULL) {
                if (SDLNet_TCP_AddSocket(this->set, this->socket) == -1) {
                    SDLNet_FreeSocketSet(this->set);
                    throw NetworkError("SDLNet_TCP_AddSocket: "
                                       + std::string(SDLNet_GetError()));
                }
                this->host = new Host(SDLNet_TCP_GetPeerAddress(socket));
            }

            /// Destructor
            virtual ~Link() {
                if (this->socket != NULL) {
                    this->close();
                }
                SDLNet_FreeSocketSet(this->set);
            }

            /// Opens the connection to a host on a port
            /**
             * Resolves the host and establishs a connection. The local port
             *  number is assigned automatically.
             *  @param host: remote hostname
             *  @param port: remote port number
             */
            void open(const std::string& host, std::uint16_t port) {
                if (this->socket != NULL) {
                    return;
                }
                this->host = new Host(host, port);
                this->socket = SDLNet_TCP_Open(this->host->addr);
                if (this->socket == NULL) {
                    throw NetworkError("SDLNet_TCP_Open: "
                                       + std::string(SDLNet_GetError()));
                }
                if (SDLNet_TCP_AddSocket(this->set, this->socket) == -1) {
                    throw NetworkError("SDLNet_TCP_AddSocket: "
                                       + std::string(SDLNet_GetError()));
                }
                this->online = true;
            }

            /// Closes the connection
            void close() {
                if (this->socket == NULL) {
                    return;
                }
                if (SDLNet_TCP_DelSocket(this->set, this->socket) == -1) {
                    throw NetworkError("SDLNet_TCP_DelSocket: "
                                       + std::string(SDLNet_GetError()));
                }
                SDLNet_TCP_Close(this->socket);
                this->socket = NULL;
                delete this->host;
                this->online = false;
                this->host = NULL;
            }

            /// Returns whether the link is online or not
            /**
             * Returns whether the link is online or not. A previously broken
             *  pipe automatically sets a link to offline
             *  @return true if online
             */
            inline bool isOnline() {
                return this->online;
            }

            /// Returns whether there is activity on the socket
            /**
             * Describes whether there is activity on the socket. This uses a
             *  socket set with this socket inside it. This method can be used
             *  to check the socket for activity to allow non-blocking receive.
             */
            inline bool isReady() {
                int numready = SDLNet_CheckSockets(set, 0);
                if (numready == -1) {
                    throw NetworkError("SDLNet_CheckSockets: "
                                       + std::string(SDLNet_GetError()));
                }
                // there is just one socket in the set
                return (numready == 1 && SDLNet_SocketReady(this->socket) != 0);
            }

            /// Send a string
            /**
             * This allows to send a string with a fixed length given by the
             *  string. If the connection is lost, this method will set the
             *  link to offline and throw a "BrokenPipe" exception.
             *  @param data: String
             */
            void write(const std::string& s) {
                if (this->socket == NULL) {
                    // tcp socket is not connected
                    this->online = false;
                    throw BrokenPipe();
                }
                // send string length
                std::uint16_t bytes = sizeof(std::uint16_t);
                std::uint16_t size = s.size();
                std::int32_t sent = SDLNet_TCP_Send(this->socket, &size, bytes);
                if (sent < (std::int32_t)bytes) {
                    // error while sending
                    this->online = false;
                    throw BrokenPipe();
                }
                // send characters
                bytes = sizeof(char) * size;
                const char* data = s.c_str();
                sent = SDLNet_TCP_Send(this->socket, data, bytes);
                if (sent < (std::int32_t)bytes) {
                    // error while sending
                    this->online = false;
                    throw BrokenPipe();
                }
            }

            /// Receive a string
            /**
             * This allows to receive a string with a fixed length, given
             *  by the socket. If the connection is lost, this method will set
             *  the link to offline and throw a "BrokenPipe" exception. It also
             *  will return NULL in this case.
             *  Consider, that this method will block until the data can be
             *  read from the socket. Use isReady() to check for incomming
             *  data to allow non-blocking receive.
             *  @param len: length of data
             *  @return pointer to data or NULL
             */
            std::string read() {
                if (this->socket == NULL) {
                    // tcp socket is not connected
                    this->online = false;
                    throw BrokenPipe();
                }
                // read string size
                std::uint16_t bytes = sizeof(std::uint16_t);
                std::uint16_t size;
                std::int32_t read = SDLNet_TCP_Recv(this->socket, &size, bytes);
                if (read <= 0) {
                    // error while receiving
                    this->online = false;
                    throw BrokenPipe();
                }
                // read characters
                bytes = sizeof(char) * size;
                char* buffer = (char*) malloc (size + 1);
                read = SDLNet_TCP_Recv(this->socket, buffer, bytes);
                if (read <= 0) {
                    // error while receiving
                    free(buffer);
                    this->online = false;
                    throw BrokenPipe();
                }
                buffer[size] = '\0';
                std::string s = std::string(buffer);
                free(buffer);
                return s;
            }

            /// Host data of the link
            Host* host;

    };

    /// A listener class for TCP links
    /**
     *  This class is listening for new TCP links.
     */
    class Listener {

        protected:
            /// server socket used for listening
            TCPsocket socket;

        public:
            /// Constructor
            Listener()
                : socket(NULL) {
            }

            /// Destructor
            virtual ~Listener(){
                if (this->socket != NULL) {
                    this->close();
                }
            }

            /// Starts listening on a local port
            /**
             * This starts the server socket on a given local port.
             *  @param port: local port number
             */
            void open(std::uint16_t port) {
                if (this->socket != NULL) {
                    return;
                }
                Host host(port);
                this->socket = SDLNet_TCP_Open(host.addr);
                if (this->socket == NULL) {
                    throw NetworkError("SDLNet_TCP_Open: "
                                       + std::string(SDLNet_GetError()));
                }
            }

            /// Returns whether the server socket is online or not
            /**
             * Returns whether the server socke4t is online or not
             *  @return true if online
             */
            inline bool isOnline() {
                return (this->socket != NULL);
            }

            /// Stops listening
            /**
             * This stops the server socket on the current local port.
             */
            void close() {
                if (this->socket == NULL) {
                    return;
                }
                SDLNet_TCP_Close(this->socket);
                this->socket = NULL;
            }

            /// Accept an incomming connection on TCP
            /**
             * This accepts an incomming connection on TCP and returns a
             *  pointer to the TcpLink. If there is no incomming connection
             *  it will return NULL.
             *  @return pointer to TcpLink or NULL
             */
            Link* accept() {
                if (this->socket == NULL) {
                    throw NetworkError("TCP Listener is not listening");
                }
                TCPsocket tmp = SDLNet_TCP_Accept(this->socket);
                if (tmp == NULL) {
                    return NULL;
                }
                return new Link(tmp);
            }

    };

}

#endif
