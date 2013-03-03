/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the EventSysteming module:
    https://github.com/cgloeckner/EventSysteming

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <SDL/SDL_net.h>

class TcpListener;

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

/// An exception class used in case of a broken connection
class ConnectionBroken: public std::exception {
    public:
        ConnectionBroken() throw() {}
        virtual ~ConnectionBroken() throw() {}
        virtual const char* what() const throw() {
            return "Connection is broken";
        }
};

/// A host with ip and port number
/**
 *  This structure is used to simplfy host resolve and getting hostname or
 *  port number in an easier way.
 */
struct Host {
    IPaddress addr;
    
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
        virtual void send(void* data, int len) = 0;
        virtual void* receive() = 0;
        template <typename Data> void send(Data* data) {
            this->send((void*)data, sizeof(Data));
        }
        template <typename Data> Data* receive() {
            return (Data*)(this->receive());
        }
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
        void send(void* data, int len);
        void* receive();
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
class UdpLink: public Link {
    protected:
        UDPsocket socket;
        unsigned long max_size;
    public:
        UdpLink();
        virtual ~UdpLink();
        void setMaxSize(unsigned long max_size);
        void setTarget(const std::string& host, unsigned short port);
        void open(unsigned short port);
        void close();
        void send(void* data, int len);
        void* receive();
};


#endif
