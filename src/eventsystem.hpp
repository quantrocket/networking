/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the EventSysteming module:
    https://github.com/cgloeckner/EventSysteming

It offers an event-based EventSysteming framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef EVENTSYSTEM_HPP
#define EVENTSYSTEM_HPP

#include <queue>
#include <SDL/SDL.h>

#include "connection.hpp"

typedef unsigned short EventID;

namespace eventid {
    /// ID for unspecified event type
    const EventID GENERIC = 0;
}

/// Base Event
struct Event {
    EventID event_id;
    Event(EventID event_id): event_id(event_id) {}
    Event(): event_id(eventid::GENERIC) {}
};


/// Helper function to start EventSystem<Link>::send_all as thread
/**
 *  @param [in] param : (void*)-casted EventSystem<Link>
 *  @return           : not used
 */
template <typename Link> int trigger_send(void* param);

/// Helper function to start EventSystem<Link>::recv_all as thread
/**
 *  @param [in] param : (void*)-casted EventSystem<Link>
 *  @return           : not used
 */
template <typename Link> int trigger_recv(void* param);


/// Template class for the EventSystem
/**
 *  The type parameter Link should be a connection class with the following
 *  methods:
 *      void send(void* data, int len);
 *      void* receive();
 *
 *  The system is pushed with several (generic) events. They are stored using
 *  the fifo-queue "outgoing". It's correct size is parallely written to
 *  "out_size". This queues' events / event-sizes are popped by the send_all()
 *  thread.
 *
 *  The system is popping events from it's internal fifo-queue "incomming".
 *  This queue's events are pushed from the recv_all() thread.
 *
 *  The send_all() thread is writing all events from "outgoing"-queue to the
 *  given link using the given "out_size" to guarantee writing the correct
 *  amount of bytes to the referring socket.
 *
 *  The recv_all() thread is reading all events (referring to it's size) from
 *  the given link and pushs it to the "incomming"-queue.
 *
 *  All queues are accessed thread-safe. All pushed Events must come from
 *  correct sub-event type to guarantee correct sending and receiving
 *  referring to network communication. All popped Events must be switch
 *  referring to it's event_id and casted to the correct sub-event type.
 */
template <typename Link>
class EventSystem {
    friend int trigger_send<Link>(void* param);
    friend int trigger_recv<Link>(void* param);
    protected:
        // incomming events
        std::queue<Event*> incomming;
        SDL_mutex* in_lock;
        // outgoing events
        std::queue<Event*> outgoing;
        std::queue<int> out_size;
        SDL_mutex* out_lock;
        // connection link
        Link* link;
        // system stuff
        bool running;
        SDL_Thread* send_thread;
        SDL_Thread* recv_thread;

        void send_all();
        void recv_all();
    public:
        EventSystem(Link* link);
        ~EventSystem();
        template <typename EventType> void push(EventType* event);
        Event* pop();
};


template <typename Link> int trigger_send(void* param) {
    EventSystem<Link>* system = (EventSystem<Link>*)param;
    system->send_all();
}

template <typename Link> int trigger_recv(void* param) {
    EventSystem<Link>* system = (EventSystem<Link>*)param;
    system->recv_all();
}

template <typename Link>
EventSystem<Link>::EventSystem(Link* link) {
    this->link    = link;
    this->running = (this->link != NULL);
    // start threads
    this->send_thread = SDL_CreateThread(trigger_send<Link>, (void*)this);
    this->recv_thread = SDL_CreateThread(trigger_recv<Link>, (void*)this);
}

template <typename Link>
EventSystem<Link>::~EventSystem() {
    // cancel threads
    SDL_KillThread(this->send_thread);
    SDL_KillThread(this->recv_thread);
    // note: link might be used outside -- should be deleted here
}

template <typename Link>
void EventSystem<Link>::send_all() {
    while (this->running) {
        // pop from outgoing queue
        SDL_LockMutex(this->out_lock);
        if (this->outgoing.empty()) {
            SDL_UnlockMutex(this->out_lock);
            SDL_Delay(15);
            continue;
        }
        Event* next = this->outgoing.front();
        this->outgoing.pop();
        int size = this->out_size.front();
        this->out_size.pop();
        SDL_UnlockMutex(this->out_lock);
        // send via link
        this->link->send(next, size);
    }
}

template <typename Link>
void EventSystem<Link>::recv_all() {
    while (this->running) {
        // receive via link
        void* next = this->link->receive();
        if (next == NULL) {
            SDL_Delay(15);
            continue;
        }
        // push to incomming queue
        SDL_LockMutex(this->in_lock);
        this->incomming.push((Event*)next);
        SDL_UnlockMutex(this->in_lock);
    }
}

// EventType must be derived from Event
template <typename Link>
template <typename EventType>
void EventSystem<Link>::push(EventType* event) {
    // push to outgoing queue
    SDL_LockMutex(this->out_lock);
    this->outgoing.push(event);
    this->out_size.push(sizeof(EventType));
    SDL_UnlockMutex(this->out_lock);
}

template <typename Link>
Event* EventSystem<Link>::pop() {
    // pop from incomming queue
    SDL_LockMutex(this->in_lock);
    if (this->incomming.empty()) {
        SDL_UnlockMutex(this->in_lock);
        return NULL;
    }
    Event* tmp = this->incomming.front();
    this->incomming.pop();
    SDL_UnlockMutex(this->in_lock);
    // return actual event
    return tmp;
}



#endif
