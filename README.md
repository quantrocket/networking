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

Currently there is no UDP support. The server-client code is completly based on TCP only. I'm not shure whether I already need UDP, so it will stay disabled for a non-specified while. Also multiple endianness is not supported. Consider this when shipping your application using the framework to different plattforms.

Please pay attention to use `float` for floating point variables in the context of JSON Objects. Currenty `double` or `long double` are not supported, because some decimal place might be cut off when serializing with `std::to_string`.

# Example

You can find a server-client-based chatroom example in the directory `example/`. It shoulds the basic usage of a possible multiuser chatroom.

# Building

Note: This Framework is a header-only framework. Just include the header-files and get ready!

Build with Code::Blocks is most simple. Just use `networking.cbp` and build as you always do. If you are using GCC without building can be done by the following line.

    g++ -o chatroom example/*.cpp -lSDL_net --std=c++0x -pthread -I./include/

I testet it using GNU/Linux Ubuntu 12.04 32-Bit and gcc 4.6.3. But it should work with MinGW as-well. Just remember too add e.g. `-lmingw32` (I had to add when compiling using Wine on my 32-bit machine.)

# Cancled Changes

I was thining about the following cancled changes, but I cannot see it's necessarity, yet:
- UDP support is not implemented. The server-client structure and the serialization-based communication needs to be redesigned when using UDP. That's too much work to too less benefit.
- Using `double` inside the JSON-objects is wasting memory. See furthur limitations for more information.
- Peer-to-peer structure is not used by me, yet. I don't have any experience implementing such a design. Feel free to offer your implementation of peer-to-peer or other networking designs.

# Possible Changes

I am thinking of the following possible changes:
- Use pointers for internal JSON-data stead of common variables. This might decrease the needed memory but might also decrease the execution speed a bit. 
- JSON is just one possibility for serialization. There might be a common interface for several kinds of serialization. Each actual implementation of this abstract serialization might be handled by the server and clients.
- The current callback-structure inside server and client is ugly. It's too "over-templated"! My vision is a seperate `CallbackManager` with a type parameter about the actual server or client. As far as I know there is now way around this while writing a framework. So the server and client might only need a pointer to the general `CallbackManager` and notify it using the predefined interface. The actual server or client will create the actual `CallbackManager` pointer - hopefully without passing through some type parameters. We'll see.

# Scheduled Changes

I am going to do the following scheduled changes:
- Use boost's asio api instead SDL_net. I am going to migrate my SDL-apps to SDL2, but SDL_net was not already modified for SDL2, yet. Also boost's asio api might increase the execution speed of sending and receiving. This should be interesting for applications with a large number of clients.
- Enable grouping clients at the server's side in a logical way. This might increase the speed for accessing clients that are often access at an event.

