Event-Based Networking Framework
============================

This file is part of the networking module https://github.com/cgloeckner/networking. The project offers an event-based networking framework for games and other
software. It's main purpose is to integrate into my own Game Projekt as well as integrate it into FLARE by Clint Bellanger one day.

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

To build the module and complex demo application using gcc you can use:

    g++ -o demo src/*.cpp example2.cpp -lSDL -lSDL_net --std=c++0x

I testet it using GNU/Linux Ubuntu 12.04 32-Bit and gcc 4.6.3.


Scheduled Changes:
----------------

    - much more testing :D 
    - extend server-client with udp stuff:
        server.enable_udp(port)
        server.disable_udp()
        tcp port given in constructor as before
        => tcp is always used, udp is optional
    - worker can open udp connection to client after tcp acception


