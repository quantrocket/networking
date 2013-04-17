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
#ifndef UTILS_INCLUDE_GUARD
#define UTILS_INCLUDE_GUARD

#include <chrono>
#include <queue>
#include <mutex>
#include <thread>

#include "json.hpp"

namespace networking {

    /// Delay some milliseconds
    /**
     * This function wraps C++11-functionallity for delays.
     *  @param ms   Amount of milliseconds to wait
     */
    inline void delay(unsigned short ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    /// Thread-Safe Queue based on JSON Objects
    /**
     * This class provides accessing a fifo queue in a thread-safe way. It can
     *  me implemented by using a specific typeparameter as base of the data.
     */
    class JsonQueue {
        protected:
            /// mutex for thread-safety of push/pop
            std::mutex mutex;
            /// actual queue with pointers
            std::queue<json::Value> data;
        public:
            /// Constructor
            JsonQueue() {
            }
            /// Destructor
            virtual ~JsonQueue() {
            }
            /// Clear all data and delete stored pointers
            inline void clear() {
                this->mutex.lock();
                while (!this->data.empty()) {
                    this->data.pop();
                }
                this->mutex.unlock();
            }
            /// Push json object to the queue
            /**
             * This pushs a given json object to the queue.
             *  @param data: json object
             */
            inline void push(json::Value data) {
                this->mutex.lock();
                this->data.push(data);
                this->mutex.unlock();
            }
            /// Pop json object from queue.
            /**
             * This pops the next json object from the queue and returns it.
             *  If the queue is empty, this function will return an empty
             *  json obejct. You can check for emptiness by isNull().
             *  @return json object
             */
            inline json::Value pop() {
                json::Value tmp;
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

    /// Json-Pipe with incomming and outgoing queue
    class JsonPipe {
        protected:
            /// Pushed Json Objects
            JsonQueue* in;
            /// Json Objects for popping
            JsonQueue* out;
        public:
            /// Constructor with given queues for incomming and outgoing events
            /**
             * Creates a json pipe using the given queues for incomming and
             *  outgoing json objects. The opposit pipe should use this'
             *  incomming queue as it's outgoing queue and this' outgoing queue
             *  as it's incomming queue:
             *      JsonQueue a, b;
             *      JsonPipe first(&a, &b);
             *      JsonPipe second(&b, &a);
             */
            JsonPipe(JsonQueue* in, JsonQueue* out)
                : in(in)
                , out(out) {
            }
            /// Destructor
            /**
             * It does not delete the incomming or outgoing queues!
             */
            virtual ~JsonPipe() {
            }
            /// Return the next json object
            /**
             * This will return the next json object from front of the outgoing
             *  queue. If the outgoing queue is empty, it will return an empty
             *  json object. It can be checked for emptiness by isNull().
             *  @result json object
             */
            inline json::Value pop() {
                return this->out->pop();
            }
            /// Add a json object
            /**
             * This will push the given json object to the end of the incomming
             *  queue.
             *  @param data: json object
             */
            inline void push(json::Value data) {
                this->in->push(data);
            }
    };

}

#endif

