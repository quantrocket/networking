/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef THREADING_HPP
#define THREADING_HPP

#include <queue>
#include <mutex>
#include <thread>
#include <chrono>

namespace networking {

    void delay(unsigned short ms);

    /// Wrapper class for mutex using std::mutex or SDL_mutex
    class Mutex {
        protected:
            std::mutex mutex;
        public:
            Mutex();
            virtual ~Mutex();
            void lock();
            void unlock();
    };

    // ------------------------------------------------------------------------

    class Thread {
        protected:std::thread thread;
        public:
            Thread();
            virtual ~Thread();

            template <typename T>
            void start(void (*func)(T*), T* param) {
                this->thread = std::thread(func, param);
            }

            void stop();
    };

    // ------------------------------------------------------------------------

    /// Thread-Safe Queue
    template <typename Data>
    class ThreadSafeQueue {
        protected:
            Mutex mutex;
            std::queue<Data*> data;
        public:
            ThreadSafeQueue() {
            }
            virtual ~ThreadSafeQueue() {
            }
            void clear() {
                this->mutex.lock();
                while (!this->data.empty()) {
                    Data* tmp = this->data.front();
                    delete tmp;
                    this->data.pop();
                }
                this->mutex.unlock();
            }
            void push(Data* data) {
                this->mutex.lock();
                this->data.push(data);
                this->mutex.unlock();
            }
            Data* pop() {
                Data* tmp = NULL;
                this->mutex.lock();
                if (!this->data.empty()) {
                    tmp = this->data.front();
                    this->data.pop();
                }
                this->mutex.unlock();
                return tmp;
            }
            bool isEmpty() {
                // seems nearly atomic to me
                return this->data.empty();
            }
    };

}

#endif

