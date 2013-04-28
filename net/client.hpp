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
#ifndef NET_CLIENT_INCLUDE_GUARD
#define NET_CLIENT_INCLUDE_GUARD

#include <set>
#include <map>
#include <cstdint>
#include <thread>

#include "link.hpp"
#include "json.hpp"
#include "common.hpp"

namespace net {

    template <typename Derived> class Client;

    /// Client
    /**
     * The client provides communication with a given server. Sending and
     *  receiving data are handled in two seperate threads.
     */
    template <typename Derived>
    class Client {

        protected:
            /// Thread for sending-receiving-loop
            std::thread networker;
            /// Thread for handle-loop
            std::thread handler;
            /// Queues
            utils::SyncQueue<json::Var> out;
            utils::SyncQueue<json::Var> in;

            /// Command-Callback Mapper
            std::map<CommandID, void (Derived::*)(json::Var&)> callbacks;
            /// Link to the server
            tcp::Link link;
            /// Client ID
            ClientID id;

            /// Fallback Handle for undefined commands
            virtual void fallback(json::Var& data) = 0;

            /// Sending-Receiving-loop
            void network_loop() {
                while (this->isOnline()) {
                    // Send All JSON Objects
                    while (true) {
                        json::Var object = this->out.pop();
                        if (object.isNull()) {
                            // Nothing Left
                            break;
                        }
                        // Serialize Object
                        std::string dump = object.dump();
                        // Send to Server
                        try {
                            this->link.write(dump);
                        } catch (const BrokenPipe& bp) {
                            std::cerr << "Connection to server was lost" << std::endl;
                            this->link.close();
                            return;
                        }
                    }
                    // Receive All JSON Objects
                    while (this->link.isReady()) {
                        std::string dump;
                        // Read Dumped Object
                        try {
                            dump = this->link.read();
                        } catch (const BrokenPipe& bp) {
                            std::cerr << "Connection to server was lost" << std::endl;
                            this->link.close();
                            return;
                        }
                        // Deserialize JSON Object
                        json::Var object;
                        object.load(dump);
                        this->in.push(object);
                    }
                    utils::delay(25);
                }
            }
            /// Handle-loop
            void handle_loop()  {
                while (this->isOnline()) {
                    // wait for next object
                    json::Var object = this->pop();
                    if (object.isNull()) {
                        // Null-Object
                        utils::delay(15);
                    } else {
                        json::Var payload = object["payload"];
                        CommandID command_id;
                        try {
                            command_id = payload["command"].getInteger();
                        } catch (json::TypeError & te) {
                            continue;
                        }
                        // Search callback
                        auto entry = this->callbacks.find(command_id);
                        if (entry == this->callbacks.end()) {
                            // Use fallback handle
                            this->fallback(payload);
                        } else {
                            // Exexcute callback
                            auto callback = entry->second;
                            Derived* ptr = static_cast<Derived*>(this);
                            (ptr->*callback)(payload);
                        }
                    }
                }
            }

        public:
            /// Constructor
            Client() {
            }

            /// Destructor
            /**
             * Will disconnect from the server
             */
            virtual ~Client() {
                this->disconnect();
            }

            /// Establish connection to the server at the given IP and port
            /**
             * Establishs a connection to the server with the given IP-address,
             *  that is listening on the given port number. This method will
             *  receive the assigned client ID and start the threads for the
             *  sending- and receiving-loops. Consider that the server might
             *  refuse a client based on it's IP-address. So be aware that this
             *  method might throw a "BrokenPipe" execption.
             *  @param ip: IP-address or hostname of the remote server
             *  @param port: port number of the remove server
             */
            void connect(std::string const & ip, std::uint16_t const port) {
                if (this->isOnline()) {
                    return;
                }
                // open connection
                this->link.open(ip, port);
                // receive client id
                std::string dump = this->link.read();
                json::Var welcome;
                welcome.load(dump);
                try {
                    this->id = welcome["id"].getInteger();
                } catch (json::TypeError const & te) {
                    throw std::runtime_error("Did not get ClientID by server");
                }
                // start threads
                this->networker = std::thread(&Client::network_loop, this);
                // start handler
                this->handler = std::thread(&Client::handle_loop, this);
            }

            /// Returns whether the client is online
            /**
             * Returns whether the client is connected to a server or not.
             *  @return true if connected
             */
            inline bool isOnline() {
                return this->link.isOnline();
            }

            /// Disconnect the current connection
            /**
             * This will close the current link to the server and safely stop
             *  the sending- and receiving-loop threads
             */
            void disconnect() {
                // close connection
                this->link.close();
                this->networker.join();
                this->handler.join();
                // clear queues
                this->in.clear();
                this->out.clear();
            }

            /// Get the next incomming bundle
            /**
             * This returns the next incomming bundle from the front of the
             *  incomming queue and pops it from there. This might be NULL if
             *  the incomming queue is empty.
             *  @return pointer to bundle or NULL
             */
            json::Var pop() {
                return this->in.pop();
            }

            /// Push an json-object for sending
            /**
             * This will create a bundle using the current client ID, the given
             *  event-pointer and the event data's size. This size is taken
             *  from the size of the type TEvent, given with the event.
             *  Consider that you should not delete this event. It is
             *  automatically deleted after sending to the server.
             *  @param event: event-pointer
             */
            void push(json::Var const & data){
                this->out.push(data);
            }

    };

}

#endif

