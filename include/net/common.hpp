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
#ifndef NET_COMMON_INCLUDE_GUARD
#define NET_COMMON_INCLUDE_GUARD

#include <iostream>
#include <chrono>
#include <queue>
#include <mutex>
#include <thread>
#include <map>

#include <signal.h>

#include <SFML/Network.hpp>

namespace net {

    /// Datatype for Command-IDs
    typedef std::uint32_t CommandID;

    /// Type for client IDs (unsigned 16-bit integer with fixed size)
    typedef std::uint32_t ClientID;

    namespace utils {
    
        /// Workaround see http://en.sfml-dev.org/forums/index.php?topic=9092.msg61423#msg61423
        inline void SocketCrashWorkaround() {
            sigset_t set;
            sigaddset(&set, SIGPIPE);
            int retcode = sigprocmask(SIG_BLOCK, &set, NULL);
            if (retcode == -1) std::cout << "sigprocmask" << std::endl;
        }

        /// Delay some milliseconds
        /**
         * This function wraps C++11-functionallity for delays.
         *  @param ms   Amount of milliseconds to wait
         */
        inline void delay(std::uint16_t const ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }

        /// Thread-Safe, Template-Based Queue
        /**
         * This class provides accessing a fifo queue in a thread-safe way. It can
         *  me implemented by using a specific typeparameter as base of the data.
         */
        template <typename Data>
        class SyncQueue {
            protected:
                /// mutex for thread-safety of push/pop
                std::mutex mutex;
                /// actual queue with pointers
                std::queue<Data> data;
            public:
                /// Constructor
                SyncQueue() {
                }
                /// Destructor
                virtual ~SyncQueue() {
                }
                /// Clear all data and delete stored pointers
                inline void clear() {
                    this->mutex.lock();
                    while (!this->data.empty()) {
                        this->data.pop();
                    }
                    this->mutex.unlock();
                }
                /// Push data to the queue
                /**
                 * This pushs a given object to the queue.
                 *  @param data: object
                 */
                inline void push(Data const & data) {
                    this->mutex.lock();
                    this->data.push(data);
                    this->mutex.unlock();
                }
                /// Pop object from queue.
                /**
                 * This pops the next object from the queue and returns it
                 *  using a reference parameter. It returns a boolean value.
                 *  Returning true means data was popped, false means no data
                 *  was obtained.
                 *  @return bool describing data was obtained or not
                 */
                inline bool pop(Data & result) {
                    this->mutex.lock();
                    bool success = !this->data.empty();
                    if (success) {
                        result = this->data.front();
                        this->data.pop();
                    }
                    this->mutex.unlock();
                    return success;
                }
                /// Non-threadsafe emptiness check
                /**
                 * This function checks for an empty internal queue. Consider that
                 *  this will not be thread-safe. But the method is quite short.
                 *  So it might me not so dangerous.
                 *  @return true if the queue is empty
                 */
                inline bool isEmpty() {
                    return this->data.empty();
                }
        };

    }
    
    /// Virtual class for your specific protocol implementation
    /**
     * First you need to derive your protocol from this abstract base class.
     *  You can completly determine in which case which data are be sent and
     *  received. Remember to use only one specific protocal at once inside
     *  the client-server structure. So your protocol should capture all your
     *  application's needs.
     *  Keep in memory to use instances of your protocol for each data package.
     */
    class BaseProtocol {

        public:
            /// Default constructor
            BaseProtocol() {}
            /// Default destructor
            virtual ~BaseProtocol() {}

            /// Virtual method for sending data using a socket
            /**
             * This must'n work blocking!
             *  @param socket: socket to use for sending
             *  @return true in case of success 
             */
            virtual bool send(sf::TcpSocket & socket) { return true; }
            /// Virtual method for receiving data using a socket
            /**
             * This must'n work blocking!
             *  @param socket: socket to use for receiving
             *  @return true in case of success
             */
            virtual bool receive(sf::TcpSocket & socket) { return true; }
            
            /// Command ID
            CommandID command;
            /// Client ID (source or target, depends on context)
            ClientID client;

    };

}

#endif

