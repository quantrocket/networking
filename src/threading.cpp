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

    Mutex::Mutex() {
        this->mutex = SDL_CreateMutex();
    }

    Mutex::~Mutex() {
        SDL_DestroyMutex(this->mutex);
    }

    void Mutex::lock() {
        SDL_LockMutex(this->mutex);
    }

    void Mutex::unlock() {
        SDL_UnlockMutex(this->mutex);
    }

    // ------------------------------------------------------------------------

    Thread::Thread()
        : thread(NULL) {
    }

    Thread::~Thread() {
        if (this->thread != NULL) {
            this->kill();
        }
    }

    void Thread::run(int (*func)(void*), void* param) {
        if (this->thread != NULL) {
            this->kill();
        }
        this->thread = SDL_CreateThread(func, param);
    }

    int Thread::wait() {
        int status = 0;
        SDL_WaitThread(this->thread, &status);
        this->thread = NULL;
        return status;
    }

    void Thread::kill() {
        SDL_KillThread(this->thread);
        this->thread = NULL;
    }

}

