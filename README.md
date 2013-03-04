Event-Based Networking Framework
============================

This file is part of the networking module https://github.com/cgloeckner/networking. The project offers an event-based networking framework for games and other
software. It's main purpose is to integrate into my own game project as well as integrate it into FLARE by Clint Bellanger one day.

The source code is released under CC BY-NC 3.0. Feel free to share and remix this work while using it for non-commercial purpose only. Please mention me as
the base of your work. Please read http://creativecommons.org/licenses/by-nc/3.0/ for further information.

Kind regards

Christian Glöckner


Limitations:
----------

Events mustn't contain pointers or other high-level data which are based on pointers. This limitation is founded in the easy way the system is sending and
receiving data over the network. There is no serialization of given events. Therefore each event has to use primitive value types only. Remember to
serialize and unserialize all data on your own before inserting these data into your derived event.


Building:
--------

To build the module and simple demo application using gcc you can use:

    g++ -o demo src/*.cpp example1.cpp -lSDL -lSDL_net --std=c++0x

To build the module and more complex demo application using gcc you can use:

    g++ -o demo src/*.cpp example2.cpp -lSDL -lSDL_net --std=c++0x

I testet it using GNU/Linux Ubuntu 12.04 32-Bit and gcc 4.6.3.


Scheduled Changes:
----------------

    - rework tcp send/receive referring to error handling
        (this might be the reason for the following bug)
    - fix bug:
        server start
        client 1 connect
        client 2 connect
        client 1 or 2 leaving
        this client reconnecting
        now the client cannot send/receive events
        disconnecting and waiting a cuple of seconds often helps
        -.-'
    - pure udp-based server-client (current server-client is pure tcp-based)


