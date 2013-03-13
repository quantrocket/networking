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
        
        Link(Host* host=NULL);
        virtual ~Link();
        virtual void close() = 0;
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
        
        void send_ptr(void* data, std::size_t len);
        void* receive_ptr(std::size_t len);

        template <typename Data> void send_data(Data data) {
            if (this->socket == NULL) {
                // tcp socket is not connected
                this->online = false;
                throw BrokenPipe();
            }
            std::size_t len = sizeof(Data);
            int sent = SDLNet_TCP_Send(this->socket, &data, len);
            if (sent < len) {
                // error while sending
                this->online = false;
                throw BrokenPipe();
            }
        }
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
    public:
        TcpListener();
        virtual ~TcpListener();
        void open(unsigned short port);
        void close();
        TcpLink* accept();
};


#endif
