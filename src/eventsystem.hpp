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

#include "threading.hpp"
#include "connection.hpp"

namespace networking {

    typedef unsigned short EventID;

    /// ID for unspecified event type
    const EventID E_GENERIC = 0;

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
     *          : Event(MY_EVENT)
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
            Event();
            // required to set correct id
            Event(EventID event_id);
            // required to copy events
            Event(Event* other);
            
            // assemble specific event from void* - buffer
            static Event* assemble(void* buffer);
    };

    /// Thread-Safe Event Queue
    class EventQueue: public ThreadSafeQueue<Event> {};

}

#endif
