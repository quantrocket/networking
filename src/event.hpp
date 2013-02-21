#ifndef EVENT_HPP
#define EVENT_HPP

#include <queue>
#include <string>
#include <SDL/SDL.h>

typedef unsigned short EventID;

namespace eventid {
    /// ID for unspecified event type
    const EventID GENERIC = 0;
}

/// Base Event
struct Event {
    EventID event_id;
    Event(EventID event_id);
    Event();
};
    
/// Thread-Safe Event-Based FIFO-Queue
/**
    Event* pop() is not blocking and might return NULL
    The stored event-pointers should be free'd while handling them.
 */
class EventPipe {
    protected:
        std::queue<Event*> events;
        SDL_mutex* lock;
   
    public:
        EventPipe();
        ~EventPipe();
        void push(Event* event);
        Event* pop();
};

#endif

