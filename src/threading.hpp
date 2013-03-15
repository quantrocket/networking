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

#include <SDL/SDL.h>

class Mutex {
    protected:
        SDL_mutex* mutex;
    public:
        Mutex();
        virtual ~Mutex();
        void lock();
        void unlock();
};

class Thread {
    protected:
        SDL_Thread* thread;
    public:
        Thread();
        virtual ~Thread();
        void run(int (*func)(void*), void* param);
        int wait();
        void kill();
};

/*
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
            bool empty;
            this->mutex.lock();
            empty = this->data.empty();
            this->mutex.unlock();
        }
};
*/

#endif

