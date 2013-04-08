# JSON-Based Networking Framework
===

This file is part of the networking module https://github.com/cgloeckner/networking. The project offers a networking framework for games and other software using json-styled data. It's main purpose is to integrate into my own game project as well as integrate it into FLARE by Clint Bellanger one day.

The source code is licenced under MIT. See COPYING for further information.

Kind regards, Christian Glöckner

# JSON over plain data
===

The first approach for my networking framework was about using plain data. I collected primitive values in structures (called "events") and sent them over the network. For correct identification it was necessary to give unique id to each event type and switch them after receiving. This whole ugly switching and typecasting can be reduced when using serialized data. Because of it's low overhead I decided to use JSON! It is also ready-to-use for maintenance and easy to read for humans.

# Current Limitations
===

Currently there is no UDP support. The server-client code is completly based on TCP only. I'm not shure whether I already need UDP, so it will stay disabled for a non-specified while. Also multiple endianness is not supported. Consider this when shipping your application using the framework to different plattforms.

Please pay attention to use `float` for floating point variables in the context of JSON Objects. Currenty `double` or `long double` are not supported, because some decimal place might be cut off when serializing with `std::to_string`.

# Example
===

You can find a server-client-based chatroom example in the directory `example/`. It shoulds the basic usage of a possible multiuser chatroom.

# Building
===

Building the module is easy; just remember to link SDL and SDL_net. Also you need to mention C++11 to your compiler as well as pthreads. You can build the example just by

    g++ -o chatroom src/*.cpp example/*.cpp -lSDL_net --std=c++0x -pthread

I testet it using GNU/Linux Ubuntu 12.04 32-Bit and gcc 4.6.3.

# Scheduled Changes
===

- event more detailed testing (e.g. for memory leaks)


