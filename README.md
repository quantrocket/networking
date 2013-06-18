# TCP-based Server-Client Framework

This file is part of the networking module at https://github.com/clgoeckner/networking. The project offers a TCP-based server-client framework for games and other applications of networking technology. It's main purpose is the integration into my own game project. But feel free to integrate it into your own project as well :) 

The source code is licenced under MIT. See COPYING for further information.

# Features

 - Multithreaded Server-Client architecture
 - TCP-based communication
 - Multi-Plattform by modern C++11 and SFML features
 - Limit number of clients (or keep open-end)
 - Block / Unlock clients based on their IP-address
 - Grouping clients to logical partitions
 - Easy-to-use: it's header-only!
 - Flexible: Use your own protocol workflow

# Dependencies

 - C++11
 - SFML 2.0

# How to use it

First I recommend to view the example given at `example/`. There you can find a very simple console-based chatroom, based on this framework. Using it in your own application is quiet easy, because it's a header-only framework!

 1. Include the header files. These can be found inside the `include/net/`-directory. All classes and stuff are assigned to the namespace `net`. So you won't not mess up your global namespace :-) 
 2. Define your protocol! The framework does not force you to use a predefined protocol. It does not offer such a protocol, at all :D Just implement your `Protocol`-class and be your own master. You can derive it from `net::BaseProtocol` and override the `send` and `receive` methods. By writing your own `send` and `receive` workflow, you can manage your communication at a basic level. You can define `CommandID`s and put all the data together you need in your application. Remember: If you choose a binary protocol, check for possible endianess problems and problems referring to 32- and 64-bit systems. In order to the second problem, I recommend to use the integer-types declared in `<cstdint>`.
 3. After finishing your protocol it's time to implement your servers and clients. Just derive from `net::Server` and `net::Client` and remember to use your protocol class as the template parameter. Then you can defined callbacks for each of your `CommandID`'s and link it to a method. Remember to implement your `fallback` handle. It is called if no other suitable callback was found.
 4. Compile and run!
 
The example application can be compiled by using:

    g++ -o chatroom example/*.cpp -std=c++0x -pthread -I./include/ -lsfml-system -lsfml-network

I testet this with GCC 4.6.4 on GNU/Linux Ubuntu 12.04 (32-bit) and SFML 2.0. Just mention to:

 - Use C++11 by `-std=c++0x`.
 - Enable threading by `-pthread`.
 - Add the `include/` directory to the include search path by `-I./include/`.
 - Link SFML by `-lsfml-system` and `-lsfml-network`.

# Current Workarounds

 - Sometimes the application crashs when trying to send data using a TCP socket
    that is already closed by remote. This is a bug within SFML. I used a
    workaround to disable this bad behavior, by using `signal.h`.
    See http://en.sfml-dev.org/forums/index.php?topic=9092.msg61423#msg61423

# Known Bugs

 - Blocking users by DNS does not work. E.g. block "127.0.0.1" instead of
    "localhost". I guess this also does not work for unblocking by DNS.

# (Maybe) Frequently Asked Questions

Q: This framework is TCP-only. Is there any UDP-support planned?

A: Yes, UDP-supported is planned!

--

Q: This framework is built on C++11. My compiler does not support C++11, yet. Can I use this framework anyway?

A: Yes and no. You need to modify some of the framework's parts by using Boost. Else you cannot use it, sorry. But feel free to fork this framework by using even more of Boost.

--

Q: This framework is built on SFML. I'm not using this I don't want to. How to use the framework in this case?

A: I plan to support multiple networking implementations as base of this Framework. SFML-Network and SDL_net will be supported in the future. I'm not shure about supporting Boost::asio, yet. Maybe :-) 

--

Q: What type of communication is supported?

A: This is up to you! The server- and client-classes are template-based and are waiting for your protocol implementation. Everything is possible: JSON, XML or even your own binary protocol formate.


