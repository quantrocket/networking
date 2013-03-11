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

#include <SDL/SDL_net.h>

class TcpListener;

/// An exception class used by a network connection
class NetworkError: public std::exception {
    protected:
        std::string msg;
    public:
        NetworkError(const std::string& msg) throw() {
            this->msg = msg;
            std::cout << "Networking error occured: " << this->msg << std::endl;
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
            std::cout << "Networking error occured: " << this->msg << std::endl;
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
    IPaddress* addr;
    
    Host(const std::string& host, unsigned short port);
    Host(unsigned short port);
    Host(IPaddress* addr);
    ~Host();
    std::string ip();
    unsigned short port();
};

// walkaround: cannot implement virtual template
// send- / receive-methods for link class
/*
typedef unsigned short LinkType;
const LinkType UNKNOWN = 0;
const LinkType TCP     = 1;
const LinkType UDP     = 2;
*/

/// An abstract class for network communication
/**
 *  The method send(void* data, int len) sends the given pointer using the
 *  given length to the link. It sends the length first, then the actual
 *  data. Using receive() will read the given length and allocate the necessary
 *  memory. It will read the actual data and return it.
 *
 *  Additionally, there are template methods for sending and receiving.
 */
class Link {
    public:
        Host* host;
        
        // walkaround: cannot implement virtual template
        // send- / receive-methods for link class
        //LinkType linktype;

        Link(Host* host=NULL);
        virtual ~Link();
        virtual void close() = 0;
        //virtual void send(void* data, std::size_t len) = 0;
        //virtual void* receive() = 0;
};

/// A link class based on a TCP socket
/**
 *  This class handles the communication using a TCP socket.
 *
 *  An instance of this class can send and receive data. The methods
 *      open(const std::string& host, unsigned int port)
 *      close()
 *  can be used to connect/disconnect the link.
 */
class TcpLink: public Link {
    friend class TcpListener;
    protected:
        TCPsocket socket;
        bool online;
    public:
        TcpLink();
        TcpLink(TCPsocket socket);
        virtual ~TcpLink();
        void open(const std::string& host, unsigned short port);
        void close();
        TcpLink* accept();
        bool isOnline();

        template <typename Data> void send(void* data) {
            this->send<Data>((Data*)data);
        }
        template <typename Data> void send(Data* data) {
            if (this->socket == NULL) {
                // tcp socket is not connected
                this->online = false;
                throw BrokenPipe();
            }
            int sent = SDLNet_TCP_Send(this->socket, data, sizeof(Data));
            if (sent < sizeof(Data)) {
                // error while sending
                this->online = false;
                throw BrokenPipe();
            }
        }
        template <typename Data> Data* receive() {
            if (this->socket == NULL) {
                // tcp socket is not connected
                this->online = false;
                throw BrokenPipe();
            }
            Data* buffer = new Data();
            int read = SDLNet_TCP_Recv(this->socket, buffer, sizeof(Data));
            if (read <= 0) {
                // error while receiving
                delete buffer;
                this->online = false;
                throw BrokenPipe();
            }
            return buffer;
        }     
};

/// A listener class for TCP links
/**
 *  This class is listening for new TCP links. You can accept incomming clients
 *  using accept(). Whether accept is blocking is determined by a bool flag
 *  that can be accessed using blocking() and blocking(bool).
 *  
 *  The methods
 *      open(unsigned short port)
 *      close()
 *  are used to start and stopp the listener.
 */
class TcpListener {
    protected:
        TCPsocket socket;
        bool _blocking;
    public:
        TcpListener();
        virtual ~TcpListener();
        void open(unsigned short port);
        void close();
        TcpLink* accept();
        void blocking(bool value);
        bool blocking();
};

/// A link class absed on a UDP socket
/**
 *  This class handles the communication using a UDP socket
 *
 *  An instance of this class can send and receive data. The methods
 *      open(const std::string& host, unsigned int port)
 *      close()
 *  can be used to connect/disconnect the link.
 *
 *  You can specify the sending target by using
 *      setTarget(const std::string& host, unsigned short port)
 *  Then all data will be send to this target until another one is specified.
 *
 *  Also you can specify the maximum size for sending or receiving packages.
 *  Both sender and receiver should use the same value to avoid data loss.
 *  Default value is 1 megabyte.
 */
/*
class UdpLink: public Link {
    protected:
        UDPsocket socket;
    public:
        UdpLink();
        virtual ~UdpLink();
        void setTarget(const std::string& host, unsigned short port);
        void open(unsigned short port);
        void close();
        /*
        void send(void* data, std::size_t len);
        void* receive();
        */
        /*
        template <typename Data> void send(Data* data) {
            if (this->socket == NULL) {
                // udp socket is not connected
                throw BrokenPipe();
            }
            // create packet
            std::size_t size = sizeof(Data);
            UDPpacket* packet = SDLNet_AllocPacket(size);
            if (packet == NULL) {
                throw NetworkError("SDLNet_AllocPacket: " + std::string(SDLNet_GetError()));
            }
            packet->channel = -1;
            memcpy(packet->data, data, size);
            packet->len     = size;
            packet->maxlen  = size;
            packet->address = this->host->addr;
            // send packet
            if (SDLNet_UDP_Send(this->socket, packet->channel, packet) == 0) {
                SDLNet_FreePacket(packet);
                throw NetworkError("SDLNet_UDP_Send: " + std::string(SDLNet_GetError()));                
            }
            SDLNet_FreePacket(packet);
        }
        template <typename Data> Data* receive() {
            // create packet
            std::size_t size = sizeof(Data);
            UDPpacket* packet = SDLNet_AllocPacket(size);
            if (packet == NULL) {
                throw NetworkError("SDLNet_AllocPacket: " + std::string(SDLNet_GetError()));
            }
            bool success = false;
            Data* buffer;
            while (!success) {
                if (this->socket == NULL) {
                    // udp socket is not connected
                    SDLNet_FreePacket(packet);
                    throw BrokenPipe();
                }
                // try to read packet
                switch (SDLNet_UDP_Recv(this->socket, packet)) {
                    case 1:
                        // packet received
                        buffer = new Data();
                        memcpy(buffer, packet->data, size);
                        SDLNet_FreePacket(packet);
                        success = true;
                        break;
                    case 0:
                        // nothing happened
                        break;
                    case -1:
                        SDLNet_FreePacket(packet);
                        throw NetworkError("SDLNet_UDP_Recv: " + std::string(SDLNet_GetError()));
                        break;
                }
            }
            return buffer;
        }
};*/


#endif
