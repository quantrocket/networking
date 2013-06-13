# JSON-Based Networking Framework

This file is part of the networking module https://github.com/cgloeckner/networking. The project offers a networking framework for games and other software using json-styled data. It's main purpose is to integrate into my own game project. An integration into FLARE by Clint Bellanger is up to him. Unfortunally the flare code seems too confusing to me, yet.

The source code is licenced under MIT. See COPYING for further information.

Kind regards, Christian Glöckner

# JSON over plain data

The first approach for my networking framework was about using plain data. I collected primitive values in structures (called "events") and sent them over the network. For correct identification it was necessary to give unique id to each event type and switch them after receiving. This whole ugly switching and typecasting can be reduced when using serialized data. Because of it's low overhead I decided to use JSON! It is also ready-to-use for maintenance and easy to read for humans.

# Dependencies

- C++11
- SDL_net (currently customized for version 1.2)

# Current Limitations

Currently there is no UDP support. The server-client code is completly based on TCP only. I'm not shure whether I already need UDP, so it will stay disabled for a non-specified while. Also multiple endianness is not supported. Consider this when shipping your application to different plattforms.

Please pay attention to use `float` for floating point variables in the context of JSON Objects. Currenty `double` or `long double` are not supported, because some decimal place might be cut off when serializing with `std::to_string`.

# Why Serialization

Of course: serialization causes overhead which might be avoid by sending old-fashon binary data through a socket.

One reason against "plain binary data" was the following: When sending e.g. different structs, I need to send some kind of identifier telling the opposite site which actual type is sent. The byte size of the struct type should be determined by this identifier. Problem: different size of datatypes. A solution might be the fixed size integer types supported by Boost and C++11. In addition to this problem there might be differences in endianess. Problem: different order of bytes within a data structure. A possible solution might be more-detailed work with different endianessess. Another reason for me was the following: If I send a structure containing several data I'd like to have a clear interface for "pushing" and "pulling" such packages to or from the network. This implies the use of a base package class as parent for all other packages, inherited from this one. Then I'd need to switch the package ident and cast my structure to another type. This also causes (small) overhead and makes the code extremly ugly, because this is going to be a framework! So I don't know which actual packages are provided by the future application. So the part of producing packages from socket data (e.g. using the factory pattern) must be separated from the framework. Elsewise the framework would destroy it's own flexibility. But this production has to be part of a framework. As a programmer using this framework I don't like to implement essential parts (like producing actual packages from the socket data). This should be done by the framework, elsewise I'd use another one which fits my needs even more.

But there are more problems: after reading data I need to create the new structure to obtain the data. A solution might be using some `alloc`/`malloc`/`calloc`. But this will force to use `free` instead of `delete` - the compiler isn't warning this at all. Memory management is a difficult topic. But forcing the "user" of the framework (that means a programmer) to use "sometimes" `free` and "sometimes" `delete` is bad design, in my opinion. It's confusion and error-prone. This should be goals to avoid with a "modern" framework using some OOP-principles.

Currently I don't to how to get rid of this problems without using serialization.

# Example

You can find a server-client-based chatroom example in the directory `example/`. It shoulds the basic usage of a possible multiuser chatroom.

# Building

Note: This Framework is easy to use: include the header files and add the cpp-files to your compilers search path.

Build with Code::Blocks is very simple. Just use `networking.cbp` and build as you always do. If you are using GCC without an IDE you can building using the following line.

    g++ -o chatroom example/*.cpp src/net/*.cpp -lSDL_net --std=c++0x -pthread -I./include/

I tested it using GNU/Linux Ubuntu 12.04 32-Bit and gcc 4.6.3. But it should work with MinGW as-well. Just remember too add e.g. `-lmingw32` (I had to add when compiling using Wine on my 32-bit machine.)

# Cancled Changes

I was thinking about the following cancled changes, but I cannot see it's necessarity, yet:
- UDP support is not implemented. The server-client structure and the serialization-based communication needs to be redesigned when using UDP. That's too much work to too less benefit.
- Using `double` inside the JSON-objects is wasting memory. See furthur limitations for more information.
- Peer-to-peer structure is not used by me, yet. I don't have any experience implementing such a design. Feel free to offer your implementation of peer-to-peer or other networking designs.
- Use Boost::Asio instead SDL_net. I'm not sure whether I really "need" this. In fact I'm not going to developing a MMORPG using this framework. But if you need more performance within the framework, feel free to offer your implementation (see `link.hpp` and `link.cpp`) using Boost::Asio.

# Possible Changes

I am thinking of the following possible changes:
- Use pointers for internal JSON-data stead of common variables. This might decrease the needed memory but might also decrease the execution speed a bit. 
- JSON is just one possibility for serialization. There might be a common interface for several kinds of serialization. Each actual implementation of this abstract serialization might be handled by the server and clients.
- Possibility to use a binary protocol. So the server/client structure is either using a text-protocol (including serialization) or a binary protocol. A problem about binary protocols might be endianess and byte size of primitive data. To make the code most flexible, text- and binary-protocols should be based on a common interface.
