/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "event.hpp"

Event::Event(EventID event_id)
    : event_id(event_id) {
}

Event::Event()
    : event_id(eventid::GENERIC) {
}

EventPipe::EventPipe() {
    this->lock = SDL_CreateMutex();
}

EventPipe::~EventPipe() {
    SDL_DestroyMutex(this->lock);
}

void EventPipe::push(Event* event) {
    SDL_LockMutex(this->lock);
    this->events.push(event);
    SDL_UnlockMutex(this->lock);
}

Event* EventPipe::pop() {
    SDL_LockMutex(this->lock);
    if (this->events.empty()) {
        SDL_UnlockMutex(this->lock);
        return NULL;
    }
    Event* tmp = this->events.front();
    this->events.pop();
    SDL_UnlockMutex(this->lock);
    // return actual event
    return tmp;
}


