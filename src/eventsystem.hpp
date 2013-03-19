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

#include <cstdint>

#include "threading.hpp"

namespace networking {

    /// Type for event IDs (unsigned 16-bit integer with fixed size)
    typedef uint16_t EventID;

    /// ID for unspecified event type
    const EventID E_GENERIC = 0;

    /// Base Event
    /**
     *  Events carry primitive data. Using pointers or high-level containers is
     *  not allowed to guarantee to serialize events referring to socket
     *  communication.
     *  Each Event-derivation needs to have an own EventID. The EventID for
     *  base events is E_GENERIC with value 0. Other EventIDs should use a
     *  non-zero, value of EventID.
     *  A derived event needs to implement a constructor calling the base
     *  event's constructor with the correct EventID. Also the
     *  "copy-constructor"-like constructor should be implemented using a
     *  pointer to enable coping events.
     */
    class Event {
        public:
            /// ID of this event
            EventID event_id;
            /// Constructor for blank event
            Event();
            /// Constructor for specific event with ID
            /**
             * Creates an event with the given event ID
             *  @param event_id: event ID
             */
            Event(EventID event_id);
            /// Constructor to create a copy from an event-pointer
            /**
             * Creates a copy of a given event-pointer's data.
             *  @param other: pointer to an event
             */
            Event(Event* other);
            /// Assemble a specific event from a void* buffer
            /**
             * This should be implemented when using this framework.
             *  It should return a new event switching it's ID and using
             *  the "copy-constructor"-like constructor of the according
             *  specialized event type.
             *  @param buffer: void-pointer buffer to event-data
             *  @return pointer to the actual event
             */
            static Event* assemble(void* buffer);
    };

    /// Thread-Safe Event Queue
    class EventQueue: public ThreadSafeQueue<Event> {};

    /// Event-Pipe with incomming and outgoing queue
    class EventPipe {
        protected:
            /// Pushed events
            EventQueue* in;
            /// Events for popping
            EventQueue* out;
        public:
            /// Constructor with given queues for incomming and outgoing events
            /**
             * Creates an event pipe using the given queues for incomming and
             *  outgoing events. The opposit pipe should use this' incomming
             *  queue as it's outgoing queue and this' outgoing queue as it's
             *  incomming queue:
             *      EventQueue a, b;
             *      EventPipe first(&a, &b);
             *      EventPipe second(&b, &a);
             */
            EventPipe(EventQueue* in, EventQueue* out);
            /// Destructor
            /**
             * It does not delete the incomming or outgoing queues!
             */
            virtual ~EventPipe();
            /// Return the next event
            /**
             * This will return the next event from front ofthe outgoing queue.
             *  If the outgoing queue is empty, it will return NULL.
             *  @result event-pointer or NULL
             */
            Event* pop();
            /// Add an event with specific type
            /**
             * This will push the given event to the end of the incomming
             *  queue.
             *  @param event: pointer to event
             */
            template <typename TEvent> void push(TEvent* event) {
                this->in->push(event);
            }
    };

}

#endif
