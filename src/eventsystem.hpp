/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef EVENTSYSTEM_HPP
#define EVENTSYSTEM_HPP

#define DELAY_ON_EMPTY 25

#include <queue>

#include <SDL/SDL.h>

#include "connection.hpp"

typedef unsigned short EventID;

namespace eventid {
    /// ID for unspecified event type
    const EventID GENERIC = 0;
}

/// Base Event
/**
 *  Events carry primitive data. Using pointers or high-level containers is
 *  not allowed to guarantee to serialize events referring to socket
 *  communication.
 *  Each Event-derivation needs to have an own EventID. The EventID for
 *  base events is eventid::GENERIC with value 0. Other EventIDs should use
 *  a non-zero, signed short integer value.
 *  A derived event needs to implement a constructor calling the base event's
 *  constructor with the correct EventID. Also die copy-constructor should be
 *  overwritten to guarantee coping events, e.g. for giving to multiple queues.
 *
 *  Here is an example about how to derive an Event:
 *
 *  namespace eventid {
 *      const EventID MY_EVENT = 1;
 *  }
 *
 *  struct MyEvent: Event {
 *      char message[255];
 *      int senderID;
 *
 *      MyEvent(const std::string& msg, int sndid)
 *          : Event(eventid::MY_EVENT)
 *          , senderID(sndid) {
 *          // copy string into char array
 *          strncpy(this->message, msg.c_str(), 255);
 *      }
 *
 *      MyEvent(const MyEvent& other)
 *          : Event(other.event_id)
 *          , senderID(other.senderID) {
 *          // copy char array
 *          strncpy(this->message, other.message, 255);
 *      }
 *  };
 */
class Event {
    public:
        EventID event_id;
        
        // required to generate base events
        Event(): event_id(eventid::GENERIC) {}
        // required to set correct id
        Event(EventID event_id): event_id(event_id) {}
        // required to copy events
        Event(Event* other): event_id(other->event_id) {}
        
        // assemble specific event from void* - buffer
        static Event* assemble(void* buffer);
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
        bool isEmpty();
};

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue()
    : lock(NULL) {
    this->lock = SDL_CreateMutex();
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
        SDL_Delay(DELAY_ON_EMPTY);
        return NULL;
    }
    T* buffer = this->data.front();
    this->data.pop();
    SDL_UnlockMutex(this->lock);
    return buffer;
}

template <typename T>
bool ThreadSafeQueue<T>::isEmpty() {
    return this->data.empty();
}

/// Simple Event Queue
class EventQueue: public ThreadSafeQueue<Event> {};

// thread helper functions
int trigger_sender(void* param);
int trigger_receiver(void* param);

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
    friend int trigger_sender(void* param);
    friend int trigger_receiver(void* param);
    protected:
        EventQueue incomming, outgoing;
        std::queue<std::size_t> outlen; // byte length of outgoing events
        SDL_mutex* outlock; // lock for pushing outgoing events
        // networking link
        TcpLink* link;
        // threading stuff
        SDL_Thread* sender_thread;
        SDL_Thread* receiver_thread;
        bool running;
    public:
        NetworkingQueue(TcpLink* link);
        ~NetworkingQueue();
        template <typename EventType> void push(EventType* event);
        Event* pop();
        bool isEmpty();
};


// EventType must be derived from Event
template <typename TEvent>
void NetworkingQueue::push(TEvent* event) {
    SDL_LockMutex(this->outlock);
    this->outgoing.push(event);
    this->outlen.push(sizeof(TEvent));
    SDL_UnlockMutex(this->outlock);
}

#endif
