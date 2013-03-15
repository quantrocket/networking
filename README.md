# THIS REPOSITORY IS CURRENTLY UNDER HUGE DEVELOPMENT. NEARLY ALL CLASSES WILL BE RE-WRITTEN FOR SIMPLICITY AND MAINTAINABILITY.


# Event-Based Networking Framework
===

This file is part of the networking module https://github.com/cgloeckner/networking. The project offers an event-based networking framework for games and other software. It's main purpose is to integrate into my own game project as well as integrate it into FLARE by Clint Bellanger one day.

The source code is released under CC BY-NC 3.0. Feel free to share and remix this work while using it for non-commercial purpose only. Please mention me as the base of your work. Please read http://creativecommons.org/licenses/by-nc/3.0/ for further information.

Kind regards

Christian Glöckner


# Limitations
===

Event customization
---

Events mustn't contain pointers or other high-level data which are based on pointers. This limitation is founded in the easy way the system is sending and receiving data over the network. There is no serialization of given events. Therefore each event has to use primitive value types only. Remember to serialize and unserialize all data on your own before inserting these data into your derived event.

From `example1.cpp`:

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


Each event needs to implement at least the default-constructor and a copy-constructor-like constructor who gets a pointer to void. Remember to set the event_id to the right value. The void-pointer-constructor is used to assemble your events from a void-pointer, as used by `Event* Event::assemble(void* buffer)`.
To customize receiving events you need to implement the static Event-method `Event* assemble(void* buffer)`. It is called by the NetworkingQueue each time the system tries to receive an event using a TCP-Link.

From `example1.cpp`:

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


Protocols
---

The primitive UDP support was removed. I'm not shure whether I already need UDP, so it will stay disabled for a non-specified while.



There will be a guide about furthur implementation nodes and more-detailed examples.


# Examples
===

`example1.cpp`
---
This example is very basic. It shows how the `NetworkingQueue` works.

`example2.cpp`
---
This example is using the single-threaded `Server` and `Client`. It provides a basic multiuser chat-system.

`example3.cpp`
---
This example is very similar to `example2.cpp`, but using the multithreading `ThreadedServer` instead of `Server`. Each worker's event can be dispatched at a single thread by `void Server::dispatch(Worker* worker, Event* event)`. In this example each dispatched events are automatically deleted by the `ThreadedServer` class after the dispatch method finished.

`example4.cpp`
---
This example is a basic game demo using "connect four" (in German: "Vier gewinnt"). It shows a very trivial implementation of network communication based on `NetworkingQueue` without any Server- or Client-classes. Currently the game does not support winning conditions. So it will run until all fields are blocked and forces the player to do a turn ... not so great but suitable as demonstration :) 


# Building
===

There are some example programs called `example1.cpp`, `example2.cpp` and so on. You can build the module and a demo application using gcc by

    g++ -o demo src/*.cpp EXAMPLE_CPP -lSDL -lSDL_net --std=c++0x

I testet it using GNU/Linux Ubuntu 12.04 32-Bit and gcc 4.6.3.


# Scheduled Changes
===

- much more detailed testing (e.g. for memory leaks)
- `NetworkingQueue`: enable waiting for receiver-thread. This is currently impossible because `SDLNet_TCP_Recv` works blocking. "A non-blocking way of using this function is to check the socket with `SDLNet_CheckSockets` and `SDLNet_SocketReady` and call `SDLNet_TCP_Recv` only if the socket is active." (see: http://sdl.beuc.net/sdl.wiki/SDLNet_TCP_Recv). "Even after the procedure started in the thread returns, there still exist some resources allocated to the thread. To free these resources, use `SDL_WaitThread` to wait for the thread to finish and obtain the status code of the thread." (see: http://sdl.beuc.net/sdl.wiki/SDL_CreateThread)


