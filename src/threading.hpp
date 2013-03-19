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

    /// Delay some milliseconds
    /**
     * This function wraps C++11-functionallity for delays.
     *  @param ms   Amount of milliseconds to wait
     */
    void delay(unsigned short ms);

    /// Lock mutual exclusive code
    /**
     * This class wraps C++11-functionallity for locking mutual exclusive code.
     *  The implementation can be changed for target systems which does not
     *  support C++11 yet.
     */
    class Mutex {
        protected:
            /// actual mutex object
            std::mutex mutex;
        public:
            /// Constructor
            Mutex();
            /// Destructor
            virtual ~Mutex();
            /// Lock the mutex
            void lock();
            /// Unlock the mutex
            void unlock();
    };

    /// Allows running a function as parallel process
    /**
     * This class wraps C++11-functionallity for calling a function as a
     *  thread. The implementation can be changed for target systems which does
     *  not support C++11 yet.
     */
    class Thread {
        protected:
            /// actual thread object
            std::thread thread;
        public:
            /// Constructor
            Thread();
            /// Destructor
            virtual ~Thread();
            /// Run a function as thread with a parameter
            /**
             * This method offers to run a function with the signature:
             *      void function(T* parameter);
             *  The type parameter T can be any type. Currently this supports
             *  only using pointers as parameters.
             *  @param param: parameter for the function
             */
            template <typename T>
            void start(void (*func)(T*), T* param) {
                this->thread = std::thread(func, param);
            }
            /// Waits for the thread and stops it.
            void wait();
    };

    // ------------------------------------------------------------------------

    /// Thread-safe, generic queue
    /**
     * This class provides accessing a fifo queue in a thread-safe way. It can
     *  me implemented by using a specific typeparameter as base of the data.
     *  Consider that the queue only holds pointers.
     */
    template <typename Data>
    class ThreadSafeQueue {
        protected:
            /// mutex for thread-safety of push/pop
            Mutex mutex;
            /// actual queue with pointers
            std::queue<Data*> data;
        public:
            /// Constructor
            ThreadSafeQueue() {}
            /// Destructor
            /**
             * The destructor does not delete the pointers inside the internal
             *  queue.
             */
            virtual ~ThreadSafeQueue() {}
            /// Clear all data and delete stored pointers
            void clear() {
                this->mutex.lock();
                while (!this->data.empty()) {
                    Data* tmp = this->data.front();
                    delete tmp;
                    this->data.pop();
                }
                this->mutex.unlock();
            }
            /// Push data pointer to the queue
            /**
             * This pushs a given pointer to the data to the queue. Consider
             *  that you should not delete this pointer after pushing! The data
             *  pointer can be safely deleted after receiving data by pop()
             *  @param data: pointer to the data
             */
            void push(Data* data) {
                this->mutex.lock();
                this->data.push(data);
                this->mutex.unlock();
            }
            /// Pop data pointer from queue - or NULL
            /**
             * This pops the next pointer from the data queue and returns it.
             *  If the queue is empty, this function will return NULL.
             *  @return pointer to data
             */
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
            /// Non-threadsafe emptiness check
            /**
             * This function checks for an empty internal queue. Consider that
             *  this will not be thread-safe. But the method is quite short.
             *  So it might me not so dangerous.
             *  @return true if the queue is empty
             */
            bool isEmpty() {
                return this->data.empty();
            }
    };

}

#endif

