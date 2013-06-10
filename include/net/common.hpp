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
#ifndef NET_COMMON_INCLUDE_GUARD
#define NET_COMMON_INCLUDE_GUARD

#include <chrono>
#include <queue>
#include <mutex>
#include <thread>
#include <map>

namespace net {

    /// Datatype for Command-IDs
    typedef std::uint32_t CommandID;

    /// Type for client IDs (unsigned 16-bit integer with fixed size)
    typedef std::uint32_t ClientID;

    namespace utils {

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
                 * This pops the next object from the queue and returns it.
                 *  If the queue is empty, this function will return an empty
                 *  obejct. You can check for emptiness by isNull().
                 *  @return object
                 */
                inline Data pop() {
                    Data tmp;
                    this->mutex.lock();
                    if (!this->data.empty()) {
                        tmp = this->data.front();
                        this->data.pop();
                    }
                    this->mutex.unlock();
                    return tmp;
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

}

#endif

