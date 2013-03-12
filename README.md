# Event-Based Networking Framework
===

This file is part of the networking module https://github.com/cgloeckner/networking. The project offers an event-based networking framework for games and other software. It's main purpose is to integrate into my own game project as well as integrate it into FLARE by Clint Bellanger one day.

The source code is released under CC BY-NC 3.0. Feel free to share and remix this work while using it for non-commercial purpose only. Please mention me as the base of your work. Please read http://creativecommons.org/licenses/by-nc/3.0/ for further information.

Kind regards

Christian Glöckner


# Limitations
===

1. Event customization

Events mustn't contain pointers or other high-level data which are based on pointers. This limitation is founded in the easy way the system is sending and receiving data over the network. There is no serialization of given events. Therefore each event has to use primitive value types only. Remember to serialize and unserialize all data on your own before inserting these data into your derived event.

from example1.cpp

    class Test: public Event {
        public:
            char text[255];
            Test()
                : Event(TEST) {
            }
            Test(Test* other)
                : Event(TEST) {
                strncpy(this->text, other->text, 255);
            }
            Test(std::string text)
                : Event(TEST) {
                strncpy(this->text, text.c_str(), 255);
            }
    };


Each event needs to implement at least the default-constructor and a copy-constructor-like constructor who gets a pointer to void. Remember to set the event_id to the right value. The void-pointer-constructor is used to assemble your events from a void-pointer, as used by Event* Event::assemble(void* buffer).
To customize receiving events to need yo implement the static Event-method Event* assemble(void* buffer). It is called by the NetworkingQueue each time the system tries to receive an event using a TCP-Link. Check the example programs for example implementations.

from example1.cpp

    Event* Event::assemble(void* buffer) {
        Event* event = reinterpret_cast<Event*>(buffer);
        EventID id = event->event_id;
        switch (id) {
            case TEST:
                event = new Test((Test*)buffer);
                break;
        };
        return event;
    }


2. Multithreading Server/Client

Currently, the classes BaseServer and BaseClient are single-threaded. Their logic can be called using the void logic() methods. Extending them for multithreading is sketched in example2.cpp and will be moved to a seperate multithreading server class.


3. Protocols

UDP support is currently disabled. I'm not shure whether I already need UDP, so it will stay disabled for a non-specified while.



There will be a guide about furthur implementation nodes and more-detailed examples.


# Building
===

To build the module and simple demo application using gcc you can use:

    g++ -o demo src/*.cpp example1.cpp -lSDL -lSDL_net --std=c++0x

To build the module and more complex demo application using gcc you can use:

    g++ -o demo src/*.cpp example2.cpp -lSDL -lSDL_net --std=c++0x

I testet it using GNU/Linux Ubuntu 12.04 32-Bit and gcc 4.6.3.


# Scheduled Changes
===

- much more detailed testing (e.g. for memory leaks)
- seperate multithreading server class


