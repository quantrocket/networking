/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the EventSysteming module:
    https://github.com/cgloeckner/EventSysteming

It offers an event-based networking framework for games and other software.

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

/// Thread-Safe Queue
/**
 *  Pushing and popping data works thread-safe. All data are stored in the
 *  data-queue.
 */
template <typename T>
class ThreadSafeQueue {
    private:
        SDL_mutex* lock;
    protected:
        std::queue<T*> data;
    public:
        ThreadSafeQueue();
        virtual ~ThreadSafeQueue();
        void push(T* data);
        T* pop();
};

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue()
    : lock(SDL_CreateMutex()) {
}

template <typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue() {
    SDL_DestroyMutex(this->lock);
}

template <typename T>
void ThreadSafeQueue<T>::push(T* data) {
    SDL_LockMutex(this->lock);
    this->data.push(data);
    SDL_UnlockMutex(this->lock);
}

template <typename T>
T* ThreadSafeQueue<T>::pop() {
    SDL_LockMutex(this->lock);
    if (this->data.empty()) {
        SDL_UnlockMutex(this->lock);
        return NULL;
    }
    T* buffer = this->data.front();
    this->data.pop();
    SDL_UnlockMutex(this->lock);
    return buffer;
}

/// Simple Event Queue
class EventQueue: public ThreadSafeQueue<Event> {};

// thread helper functions
int trigger_send(void* param);
int trigger_recv(void* param);

/// Networking Event Queue
/**
 *  The system is pushed with several specialized events. They are stored using
 *  the outgoing-queue. It's correct size is parallely written to
 *  size-queue. This queues' events / event-sizes are popped by the send_all()
 *  thread. All data are pushed thread-safe
 *
 *  The system is popping events from it's internal incomming-queue, which
 *  offers thread-safe methods. This queue's events are pushed from the
 *  recv_all() thread.
 *
 *  The send_all() thread is writing all events from "outgoing"-queue to the
 *  given link using the given "out_size" to guarantee writing the correct
 *  amount of bytes to the referring socket.
 *
 *  The recv_all() thread is reading all events (referring to it's size) from
 *  the given link and pushs it to the "incomming"-queue.
 *
 *  All pushed Events must come from sub-event typed variables to guarantee
 *  correct sending and receiving referring to network communication. All
 *  popped Events must be switch referring to it's event_id and casted to the
 *  actual sub-event type.
 */
class NetworkingQueue {
    friend int trigger_send(void* param);
    friend int trigger_recv(void* param);
    protected:
        EventQueue incomming;
        // outgoing events 
        std::queue<Event*> outgoing;
        std::queue<std::size_t> size;
        SDL_mutex* lock;
        // networking link
        Link* link;
        // threading stuff
        bool running;
        SDL_Thread* send_thread;
        SDL_Thread* recv_thread;
        void send_all();
        void recv_all();
    public:
        NetworkingQueue(Link* link);
        ~NetworkingQueue();
        template <typename EventType> void push(EventType* event);
        Event* pop();
};


// EventType must be derived from Event
template <typename EventType>
void NetworkingQueue::push(EventType* event) {
    // push data and size to outgoing queue
    SDL_LockMutex(this->lock);
    this->outgoing.push(event);
    this->size.push(sizeof(EventType));
    SDL_UnlockMutex(this->lock);
}


#endif
