/*
Copyright (c) 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers a tcp-based server-client framework for games and other software.

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
#ifndef NET_CLIENT_INCLUDE_GUARD
#define NET_CLIENT_INCLUDE_GUARD

#include <set>
#include <map>
#include <cstdint>
#include <thread>

#include <SFML/Network.hpp>

#include <net/common.hpp>
#include <net/callbacks.hpp>

namespace net {

    /// Client
    /**
     * The client provides communication with a given server. Sending and
     *  receiving data are handled in two seperate threads.
     */
    template <typename Protocol>
    class Client: public CallbackManager<CommandID, Protocol &> {

        protected:
            /// Threads
            std::thread networker;
            std::thread handler;
            /// Queues
            utils::SyncQueue<Protocol> out;
            utils::SyncQueue<Protocol> in;

            /// Link to the server
            sf::TcpSocket link;
            /// Client ID
            ClientID id;

            /// Send / Receive next data
            bool sendNext();
            bool receiveNext();

            /// Threaded loops
            void network_loop();
            void handle_loop();

        public:
            /// Constructor
            Client();

            /// Destructor
            /**
             * Will disconnect from the server
             */
            virtual ~Client();

            /// Establish connection to the server at the given IP and port
            /**
             *  @param ip: IP-address or hostname of the remote server
             *  @param port: port number of the remove server
             *  @return: true for success
             */
            bool connect(std::string const & ip, std::uint16_t const port);

            /// Returns whether the client is online
            /**
             *  @return true if connected
             */
            inline bool isOnline() {
                return (this->link.getRemoteAddress() != sf::IpAddress::None);
            }

            /// Disconnect safely
            /**
             * This will wait for an empty outgoing queue and then disconnects.
             */
            virtual void shutdown();

            /// Disconnect the current connection
            /**
             * This will close the current link to the server and safely stop
             *  the thread loops.
             */
            void disconnect();

            /// Push data for sending
            /**
             *  @param data: data
             */
            inline void push(Protocol & data){
                this->out.push(data);
            }

    };
    
    template <typename Protocol>
    Client<Protocol>::Client()
        : CallbackManager<CommandID, Protocol &>() {
        // Workaround for Tcp Socket Crash
        utils::SocketCrashWorkaround();
    }

    template <typename Protocol>
    Client<Protocol>::~Client() {
        if (this->isOnline()) {
            this->disconnect();
        }
    }
    
    template <typename Protocol>
    bool Client<Protocol>::sendNext() {
        // Pick next from outgoing queue
        Protocol object;
        if (!this->out.pop(object)) {
            return false;
        }
        // Send to server
        if (!object.send(this->link)) {
            // Cannot send
            if (!this->isOnline()) {
                // Pipe broken
                this->link.disconnect();
            }
            return false;
        }
        return true;
    }

    template <typename Protocol>
    bool Client<Protocol>::receiveNext() {
        // Try to receive next
        Protocol object;
        if (!object.receive(this->link)) {
            // Cannot receive
            if (!this->isOnline()) {
                // Pipe broken
                this->link.disconnect();
            }
            return false;
        }
        // Push to incomming queue
        this->in.push(object);
        return true;
    }

    template <typename Protocol>
    void Client<Protocol>::network_loop() {
        do {
            // Send all objects
            while (this->sendNext()) {}
            // Receive all objects
            while (this->receiveNext()) {}
            // delay a bit
            utils::delay(25);
        } while (this->isOnline());
        std::cerr << "Connection to the server was lost" << std::endl
                  << std::flush;
    }

    template <typename Protocol>
    void Client<Protocol>::handle_loop()  {
        do {
            // Pick next from incomming queue
            Protocol object;            
            if (!this->in.pop(object)) {
                // Nothing to do
                utils::delay(25);
                continue;
            }
            // Trigger callback method
            this->trigger(object.command, object);
        } while (this->isOnline());
    }

    template <typename Protocol>
    bool Client<Protocol>::connect(std::string const & ip, std::uint16_t const port) {
        if (this->isOnline()) {
            // Already connected
            return true;
        }
        // Connect
        auto status = this->link.connect(sf::IpAddress(ip), port);
        if (status != sf::Socket::Done) {
            return false;
        }
        // Receive ClientID
        sf::Packet packet;
        status = this->link.receive(packet);
        if (status != sf::Socket::Done) {
            // Got invalid ClientID or nothing
            this->link.disconnect();
            return false;
        }
        packet >> this->id;
        std::cerr << "Authed as #" << this->id << " by the server at " << ip
                  << ":" << port << std::endl << std::flush;
        this->link.setBlocking(false);
        // Start Threads
        this->networker = std::thread(&Client<Protocol>::network_loop, this);
        this->handler   = std::thread(&Client<Protocol>::handle_loop, this);
        return true;
    }

    template <typename Protocol>
    void Client<Protocol>::shutdown() {
        // wait until outgoing queue is empty
        // @note: data that is pushed while this queue is waiting might be lost
        while (this->isOnline() && !this->out.isEmpty()) {
            utils::delay(15);
        }
        this->disconnect();
    }

    template <typename Protocol>
    void Client<Protocol>::disconnect() {
        // close connection
        this->link.disconnect();
        // shutdown threads (try-catched, because they might have been stopped, yet)
        try {
            this->networker.join();
        } catch (std::system_error const & se) {}
        try {
            this->handler.join();
        } catch (std::system_error const & se) {}
        // clear queues
        this->in.clear();
        this->out.clear();
    }

}

#endif

