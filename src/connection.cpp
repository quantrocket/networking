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
    : addr(addr) {
}

Host::~Host() {
    delete this->addr;
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
    : host(host)
    /*, linktype(UNKNOWN)*/ {
}

Link::~Link() {
}

// ----------------------------------------------------------------------------

TcpLink::TcpLink()
    : Link(NULL)
    , socket(NULL)
    , online(false) {
    //this->linktype = TCP;
}

TcpLink::TcpLink(TCPsocket socket)
    : Link(NULL)
    , socket(socket)
    , online(true) {
    //this->linktype = TCP;
    this->host = new Host(SDLNet_TCP_GetPeerAddress(socket));
}

TcpLink::~TcpLink() {
    if (this->socket != NULL) {
        this->close();
    }
}

void TcpLink::open(const std::string& host, unsigned short port) {
    if (this->socket != NULL) {
        std::cout << "TCP Socket already connected" << std::endl;
        return;
    }
    this->host = new Host(host, port);
    this->socket = SDLNet_TCP_Open(this->host->addr);
    if (this->socket == NULL) {
        throw NetworkError("SDLNet_TCP_Open: " + std::string(SDLNet_GetError()));
    }
    this->online = true;
}

void TcpLink::close() {
    if (this->socket == NULL) {
        std::cout << "TCP Socket already closed" << std::endl;
        return;
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

/*
void TcpLink::send(void* data, std::size_t len) {
    if (this->socket == NULL) {
        std::cout << "TCP Socket is not connected" << std::endl;
        this->online = false;
        throw BrokenPipe();
    }
    // write byte amount
    std::size_t expected = sizeof(std::size_t);
    int sent = SDLNet_TCP_Send(this->socket, &len, expected);
    if (sent < expected) {
        this->online = false;
        throw BrokenPipe();
    }
    // write actual data
    sent = SDLNet_TCP_Send(this->socket, data, len);
    if (sent < len) {
        this->online = false;
        throw BrokenPipe();
    }
}

void* TcpLink::receive() {
    if (this->socket == NULL) {
        std::cout << "TCP Socket is not connected" << std::endl;
        this->online = false;
        throw BrokenPipe();
    }
    // read byte amount
    std::size_t len;
    int read = SDLNet_TCP_Recv(this->socket, &len, sizeof(std::size_t));
    if (read <= 0) {
        this->online = false;
        throw BrokenPipe();
    } else if (len == 0) {
        std::cout << "Package with zero size received from tcp socket of "
                  << this->host->ip() << ":" << this->host->port() << std::endl;
        return NULL;
    }
    // allocate memory
    this->last_len = len;
    void* buffer = malloc(len);
    // read actual data
    read = SDLNet_TCP_Recv(this->socket, buffer, len);
    if (read <= 0) {
        this->online = false;
        free(buffer);
        throw BrokenPipe();
    }
    return buffer;
}
*/

// ---------------------------------------------------------------------------- 

TcpListener::TcpListener()
    : socket(NULL)
    , _blocking(false) {
}

TcpListener::~TcpListener() {
    if (this->socket != NULL) {
        this->close();
    }
}

void TcpListener::open(unsigned short port) {
    if (this->socket != NULL) {
        std::cout << "TCP Listener already listening" << std::endl;
        return;
    }
    Host host(port);
    this->socket = SDLNet_TCP_Open(host.addr);
    if (this->socket == NULL) {
        throw NetworkError("SDLNet_TCP_Open: " + std::string(SDLNet_GetError()));
    }
}

void TcpListener::close() {
    if (this->socket == NULL) {
        std::cout << "TCP Listener already closed" << std::endl;
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

/*
// ---------------------------------------------------------------------------- 

UdpLink::UdpLink()
    : Link(NULL)
    , socket(NULL) {
    this->linktype = UDP;
}

UdpLink::~UdpLink() {
    if (this->socket != NULL) {
        this->close();
    }
}

void UdpLink::setTarget(const std::string& host, unsigned short port) {
    if (this->host != NULL) {
        delete this->host;
    }
    this->host = new Host(host, port);
}

void UdpLink::open(unsigned short port) {
    if (this->socket != NULL) {
        std::cout << "UDP Socket already connected" << std::endl;
        return;
    }
    this->socket = SDLNet_UDP_Open(port);
    if (this->socket == NULL) {
        throw NetworkError("SDLNet_UDP_Open: " + std::string(SDLNet_GetError()));
    }
}

void UdpLink::close() {
    if (this->socket == NULL) {
        std::cout << "UDP Socket already closed" << std::endl;
        return;
    }
    SDLNet_UDP_Close(this->socket);
    this->socket = NULL;
    if (this->host != NULL) {
        delete this->host;
        this->host = NULL;
    }
}
*/

/*
void UdpLink::send(void* data, std::size_t len) {
    if (this->socket == NULL) {
        std::cout << "UDP Socket is not connected" << std::endl;
        throw BrokenPipe();
    }
    // create package
    UDPpacket* packet = SDLNet_AllocPacket(this->max_size);
    if (packet == NULL) {
        throw NetworkError("SDLNet_AllocPacket: " + std::string(SDLNet_GetError()));
    }
    packet->channel = -1;
    memcpy(packet->data, data, len);
    packet->len     = len;
    packet->maxlen  = this->max_size;
    packet->address = this->host->addr;
    // send package
    if (SDLNet_UDP_Send(this->socket, packet->channel, packet) == 0) {
        throw NetworkError("SDLNet_UDP_Send: " + std::string(SDLNet_GetError()));
    }
    SDLNet_FreePacket(packet);
}

void* UdpLink::receive() {
    bool success = false;
    void* data = NULL;
    // prepare package
    UDPpacket* packet = SDLNet_AllocPacket(this->max_size);
    if (packet == NULL) {
        throw NetworkError("SDLNet_AllocPacket: " + std::string(SDLNet_GetError()));
    }
    while (!success) {
        if (this->socket == NULL) {
            std::cout << "UDP Socket is not connected" << std::endl;
            SDLNet_FreePacket(packet);
            throw BrokenPipe();
        }
        // try to read package
        switch (SDLNet_UDP_Recv(this->socket, packet)) {
            case 1:
                // package received
                data = malloc(packet->len);
                this->last_len = packet->len;
                memcpy(data, packet->data, packet->len);
                success = true;
                break;
            case 0:
                // nothing happened
                break;
            case -1:
                // error occured
                SDLNet_FreePacket(packet);
                free(data);
                throw NetworkError("SDLNet_UDP_Recv: " + std::string(SDLNet_GetError()));
        }
    }
    SDLNet_FreePacket(packet);
    return data;
}
*/        

