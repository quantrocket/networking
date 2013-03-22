# Event-Based Networking Framework
===

This file is part of the networking module https://github.com/cgloeckner/networking. The project offers an event-based networking framework for games and other software. It's main purpose is to integrate into my own game project as well as integrate it into FLARE by Clint Bellanger one day.

The source code is released under CC BY-NC 3.0. Feel free to share and remix this work while using it for non-commercial purpose only. Please mention me as the base of your work. Please read http://creativecommons.org/licenses/by-nc/3.0/ for further information.

Kind regards, Christian Glöckner

# Customizing events
===

Events mustn't contain pointers or other high-level data which are based on pointers. This limitation is founded in the easy way the system is sending and receiving data over the network. There is no serialization of given events. Therefore each event has to use primitive value types only. Remember to serialize and unserialize all data on your own before inserting these data into your derived event.

Each event needs to implement at least the default-constructor and a copy-constructor-like constructor who gets a pointer to void. Remember to set the event_id to the right value. The void-pointer-constructor is used to assemble your events from a void-pointer, as used by `Event* Event::assemble(void* buffer)`.
To customize receiving events you need to implement the static Event-method `Event* assemble(void* buffer)`. It is called by the NetworkingQueue each time the system tries to receive an event using a TCP-Link.

There is a `generate.py` and an `example.cfg` to easily create customized events. At the moment the python-code is ugly and might be not easy to read. This will be improved in future.

# Futhur limitations
===

Currently there is no UDP support. The server-client code is completly based on TCP only. I'm not shure whether I already need UDP, so it will stay disabled for a non-specified while. Also multiple endianness is not supported. Consider this when shipping your application using the framework to different plattforms.

# Example
===

You can find a server-client-based chatroom example in the directory `example/`. It shoulds the basic usage of a possible multiuser chatroom.

# Building
===

Building the module is easy; just remember to link SDL and SDL_net. Also you need to mention C++11 to your compiler as well as pthreads. You can build the example just by

    g++ -o chatroom src/*.cpp example/*.cpp -lSDL -lSDL_net --std=c++0x -pthread

I testet it using GNU/Linux Ubuntu 12.04 32-Bit and gcc 4.6.3.

# Scheduled Changes
===

- event more detailed testing (e.g. for memory leaks)
- improved code structure of `generate.py`

