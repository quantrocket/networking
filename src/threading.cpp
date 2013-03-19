/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "threading.hpp"

namespace networking {

    void delay(unsigned short ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    Mutex::Mutex() {
    }

    Mutex::~Mutex() {
    }

    void Mutex::lock() {
        this->mutex.lock();
    }

    void Mutex::unlock() {
        this->mutex.unlock();
    }

    // ------------------------------------------------------------------------

    Thread::Thread() {
    }

    Thread::~Thread() {
        if (this->thread.joinable()) {
            this->thread.join();
        }
    }

    void Thread::stop() {
        try {
            this->thread.join();
        } catch (const std::system_error& se) {
            // ...
        }
    }

}

