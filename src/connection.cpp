/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "connection.hpp"

Host::Host(const std::string& host, unsigned short port)
    : addr(new IPaddress()) {
    if (SDLNet_ResolveHost(this->addr, host.c_str(), port) == -1) {
        throw NetworkError("SDLNet_ResolveHost: " + std::string(SDLNet_GetError()));
    }
}

Host::Host(unsigned short port)
    : addr(new IPaddress()) {
    if (SDLNet_ResolveHost(this->addr, NULL, port) == -1) {
        throw NetworkError("SDLNet_ResolveHost: " + std::string(SDLNet_GetError()));
    }
}

Host::Host(IPaddress* addr)
    : addr(new IPaddress()) {
    this->addr->host = addr->host;
    this->addr->port = addr->port;
}

Host::~Host() {
    if (this->addr != NULL) {
        delete this->addr;
    }
}

std::string Host::ip() {
    Uint32 number = SDLNet_Read32(&(this->addr->host));
    int a, b, c, d;
    a = (number & 0xFF000000) >> 24;
    b = (number & 0x00FF0000) >> 16;
    c = (number & 0x0000FF00) >> 8;
    d = (number & 0x000000FF);
    return std::to_string(a) + "." + std::to_string(b) + "."
            + std::to_string(c) + "." + std::to_string(d);
}

unsigned short Host::port() {
    return SDLNet_Read16(&(this->addr->port));
}

// ----------------------------------------------------------------------------

Link::Link(Host* host)
    : host(host) {
}

Link::~Link() {
}

// ----------------------------------------------------------------------------

TcpLink::TcpLink()
    : Link(NULL)
    , set(SDLNet_AllocSocketSet(1))
    , socket(NULL)
    , online(false) {
}

TcpLink::TcpLink(TCPsocket socket)
    : Link(NULL)
    , set(SDLNet_AllocSocketSet(1))
    , socket(socket)
    , online(true) {
    if (SDLNet_TCP_AddSocket(this->set, this->socket) == -1) {
        SDLNet_FreeSocketSet(this->set);
        throw NetworkError("SDLNet_TCP_AddSocket: " + std::string(SDLNet_GetError()));
    }
    this->host = new Host(SDLNet_TCP_GetPeerAddress(socket));
}

TcpLink::~TcpLink() {
    if (this->socket != NULL) {
        this->close();
    }
    SDLNet_FreeSocketSet(this->set);
}

void TcpLink::open(const std::string& host, unsigned short port) {
    if (this->socket != NULL) {
        return;
    }
    this->host = new Host(host, port);
    this->socket = SDLNet_TCP_Open(this->host->addr);
    if (this->socket == NULL) {
        throw NetworkError("SDLNet_TCP_Open: " + std::string(SDLNet_GetError()));
    }
    if (SDLNet_TCP_AddSocket(this->set, this->socket) == -1) {
        throw NetworkError("SDLNet_TCP_AddSocket: " + std::string(SDLNet_GetError()));
    }
    this->online = true;
}

void TcpLink::close() {
    if (this->socket == NULL) {
        return;
    }
    if (SDLNet_TCP_DelSocket(this->set, this->socket) == -1) {
        throw NetworkError("SDLNet_TCP_DelSocket: " + std::string(SDLNet_GetError()));
    }
    SDLNet_TCP_Close(this->socket);
    this->socket = NULL;
    delete this->host;
    this->online = false;
    this->host = NULL;
}

bool TcpLink::isOnline() {
    return this->online;
}

bool TcpLink::isReady() {
    int numready = SDLNet_CheckSockets(set, 0);
    if (numready == -1) {
        throw NetworkError("SDLNet_CheckSockets: " + std::string(SDLNet_GetError()));
    }
    // there is just one socket in the set
    return (numready == 1 && SDLNet_SocketReady(this->socket) != 0);
}

void TcpLink::send_ptr(void* data, std::size_t len) {
    if (this->socket == NULL) {
        // tcp socket is not connected
        this->online = false;
        throw BrokenPipe();
    }
    int sent = SDLNet_TCP_Send(this->socket, data, len);
    if (sent < len) {
        // error while sending
        this->online = false;
        throw BrokenPipe();
    }
}

void* TcpLink::receive_ptr(std::size_t len) {
    if (this->socket == NULL) {
        // tcp socket is not connected
        this->online = false;
        throw BrokenPipe();
    }
    void* buffer = malloc(len);
    int read = SDLNet_TCP_Recv(this->socket, buffer, len);
    if (read <= 0) {
        // error while receiving
        free(buffer);
        this->online = false;
        throw BrokenPipe();
    }
    return buffer;
}

// ---------------------------------------------------------------------------- 

TcpListener::TcpListener()
    : socket(NULL) {
}

TcpListener::~TcpListener() {
    if (this->socket != NULL) {
        this->close();
    }
}

void TcpListener::open(unsigned short port) {
    if (this->socket != NULL) {
        return;
    }
    Host host(port);
    this->socket = SDLNet_TCP_Open(host.addr);
    if (this->socket == NULL) {
        throw NetworkError("SDLNet_TCP_Open: " + std::string(SDLNet_GetError()));
    }
}

bool TcpListener::isOnline() {
    return (this->socket != NULL);
}

void TcpListener::close() {
    if (this->socket == NULL) {
        return;
    }
    SDLNet_TCP_Close(this->socket);
    this->socket = NULL;
}

TcpLink* TcpListener::accept() {
    if (this->socket == NULL) {
        throw NetworkError("TCP Listener is not listening");
    }
    TCPsocket tmp = SDLNet_TCP_Accept(this->socket);
    if (tmp == NULL) {
        return NULL;
    }
    return new TcpLink(tmp);
}

