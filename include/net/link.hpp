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
#ifndef NET_LINK_INCLUDE_GUARD
#define NET_LINK_INCLUDE_GUARD

#include <iostream>
#include <stdlib.h>
#include <cstdint>

#include <SDL/SDL_net.h>

namespace net {

    class Listener;

    /// An exception class used by a network connection
    class NetworkError: public std::exception {
        protected:
            std::string msg;
        public:
            NetworkError(std::string const & msg) throw() {
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

    namespace tcp {

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
                /// IP address data
                IPaddress addr;

            public:
                /// Constructor for an offline link
                Link();

                /// Constructor for an online, given link
                /**
                 * Creates the link from a given TCP-socket.
                 *  @param socket: TCP-socket to use
                 */
                Link(TCPsocket const socket);

                /// Destructor
                virtual ~Link();

                /// Opens the connection to a host on a port
                /**
                 * Resolves the host and establishs a connection. The local port
                 *  number is assigned automatically.
                 *  @param host: remote hostname
                 *  @param port: remote port number
                 */
                void open(std::string const & host, std::uint16_t const port);

                /// Closes the connection
                void close();

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
                    return (numready == 1 &&
                            SDLNet_SocketReady(this->socket) != 0);
                }

                /// Send a string
                /**
                 * This allows to send a string with a fixed length given by the
                 *  string. If the connection is lost, this method will set the
                 *  link to offline and throw a "BrokenPipe" exception.
                 *  @param data: String
                 */
                void write(std::string const & s);

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
                std::string read();

                /// Return the remote host's address
                /**
                 * @return IPv4-address as string
                 */
                inline std::string getHost() {
                    Uint32 number = SDLNet_Read32(&(this->addr.host));
                    int a, b, c, d;
                    a = (number & 0xFF000000) >> 24;
                    b = (number & 0x00FF0000) >> 16;
                    c = (number & 0x0000FF00) >> 8;
                    d = (number & 0x000000FF);
                    return std::to_string(a) + "." + std::to_string(b) + "."
                            + std::to_string(c) + "." + std::to_string(d);
                }

                /// Return the remote host's port number
                /**
                 * @return port number as unsigned 16-bit integer
                 */
                inline std::uint16_t port() {
                    return SDLNet_Read16(&(this->addr.port));
                }

        };

        /// A listener class for TCP links
        /**
         *  This class is listening for new TCP links.
         */
        class Listener {

            protected:
                /// server socket used for listening
                TCPsocket socket;
                /// IP address data
                IPaddress addr;

            public:
                /// Constructor
                Listener();

                /// Destructor
                virtual ~Listener();

                /// Starts listening on a local port
                /**
                 * This starts the server socket on a given local port.
                 *  @param port: local port number
                 */
                void open(std::uint16_t const port);

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
                void close();

                /// Accept an incomming connection on TCP
                /**
                 * This accepts an incomming connection on TCP and returns a
                 *  pointer to the TcpLink. If there is no incomming connection
                 *  it will return NULL.
                 *  @return pointer to TcpLink or NULL
                 */
                Link* accept();

        };

    }

}

#endif
