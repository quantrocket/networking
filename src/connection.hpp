/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the EventSysteming module:
    https://github.com/cgloeckner/EventSysteming

It offers an event-based EventSysteming framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <iostream>
//#include <stdexcept>
#include <SDL/SDL_net.h>

/// An exception class used by a network connection
class NetworkError: public std::exception {
    protected:
        std::string msg;
    public:
        NetworkError(const std::string& msg) throw() {
            this->msg = msg;
        }
        virtual ~NetworkError() throw() {}
        virtual const char* what() const throw() {
            return this->msg.c_str();
        }
};


/// A connection class based on a TCP socket
/**
 *  This class handles the communication using a TCP socket. It can either
 *  be a server socket (listen and accept incomming clients) or a typical
 *  send-receive-socket. So you can use the following functions:
 *      listen(unsigned short port)
 *      connect(const std::string& host, unsigned int port)
 *  To accept or disconnect you can use the accept() and disconnect() methods.
 *
 *  Using send(void* data, int len) sends the given pointer using the given
 *  length to the socket. It sends the length first, then the actual data.
 *  Using receive() will read the given length and then the actual data. It
 *  will allocate the memory and return the read data.
 *
 *  Nearly all methods can throw exception of the type NetworkError. Usually
 *  they should not be thrown. But for handle eventual possible errors it's
 *  better to handle them always.
 */
class TcpConnection {
    protected:
        bool serve;
        TCPsocket socket;
    public:
        TcpConnection();
        TcpConnection(TCPsocket socket);
        ~TcpConnection();
        void listen(unsigned short port);
        void connect(const std::string& host, unsigned short port);
        void disconnect();
        TcpConnection* accept();
        void send(void* data, int len);
        void* receive();
};

// todo: UdpConnection class


#endif
