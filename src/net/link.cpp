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

#include <net/link.hpp>

namespace net {

    namespace tcp {

        Link::Link()
            : set(SDLNet_AllocSocketSet(1))
            , socket(NULL)
            , online(false) {
        }

        Link::Link(TCPsocket const socket)
            : set(SDLNet_AllocSocketSet(1))
            , socket(socket)
            , online(true) {
            if (SDLNet_TCP_AddSocket(this->set, this->socket) == -1) {
                SDLNet_FreeSocketSet(this->set);
                throw NetworkError("SDLNet_TCP_AddSocket: "
                                   + std::string(SDLNet_GetError()));
            }
            auto addr = SDLNet_TCP_GetPeerAddress(socket);
            this->addr.host = addr->host;
            this->addr.port = addr->port;
        }

        Link::~Link() {
            if (this->socket != NULL) {
                this->close();
            }
            SDLNet_FreeSocketSet(this->set);
        }

        void Link::open(std::string const & host, std::uint16_t const port) {
            if (this->socket != NULL) {
                return;
            }
            if (SDLNet_ResolveHost(&(this->addr), host.c_str(), port) == -1) {
                throw NetworkError("SDLNet_ResolveHost: "
                                   + std::string(SDLNet_GetError()));
            }
            this->socket = SDLNet_TCP_Open(&(this->addr));
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

        void Link::close() {
            if (this->socket == NULL) {
                return;
            }
            if (SDLNet_TCP_DelSocket(this->set, this->socket) == -1) {
                throw NetworkError("SDLNet_TCP_DelSocket: "
                                   + std::string(SDLNet_GetError()));
            }
            SDLNet_TCP_Close(this->socket);
            this->socket = NULL;
            this->online = false;
        }

        void Link::write(std::string const & s) {
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
            char const * data = s.c_str();
            sent = SDLNet_TCP_Send(this->socket, data, bytes);
            if (sent < (std::int32_t)bytes) {
                // error while sending
                this->online = false;
                throw BrokenPipe();
            }
        }

        std::string Link::read() {
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

        // --------------------------------------------------------------------

        Listener::Listener()
            : socket(NULL) {
        }

        Listener::~Listener(){
            if (this->socket != NULL) {
                this->close();
            }
        }

        void Listener::open(std::uint16_t const port) {
            if (this->socket != NULL) {
                return;
            }
            if (SDLNet_ResolveHost(&(this->addr), NULL, port) == -1) {
                throw NetworkError("SDLNet_ResolveHost: "
                                   + std::string(SDLNet_GetError()));
            }
            this->socket = SDLNet_TCP_Open(&(this->addr));
            if (this->socket == NULL) {
                throw NetworkError("SDLNet_TCP_Open: "
                                   + std::string(SDLNet_GetError()));
            }
        }

        void Listener::close() {
            if (this->socket == NULL) {
                return;
            }
            SDLNet_TCP_Close(this->socket);
            this->socket = NULL;
        }

        Link* Listener::accept() {
            if (this->socket == NULL) {
                throw NetworkError("TCP Listener is not listening");
            }
            TCPsocket tmp = SDLNet_TCP_Accept(this->socket);
            if (tmp == NULL) {
                return NULL;
            }
            return new Link(tmp);
        }

    }

}
