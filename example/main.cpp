/*
Copyright (c) 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers a tcp-based server-client framework for games and other software.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <iostream>

#include "commands.hpp"
#include "chatclient.hpp"
#include "chatserver.hpp"

void servermode(std::uint16_t port) {
    ChatServer server(port);
    
    std::string input;
    while (server.isOnline()) {
        getline(std::cin, input);
        if (input == "quit") {
            server.request_logout();
            break;
        }
    }
    server.shutdown();
}

void clientmode(std::string const & host, std::uint16_t port) {
    std::cout << "Username: ";
    std::string input;
    getline(std::cin, input);

    ChatClient client(host, port);
    client.request_login(input);

    while (client.isOnline()) {
        getline(std::cin, input);
        if (!client.authed) { continue; }
        if (input == "quit") {
            client.request_logout();
        } else {
            client.request_message(input);
        }
    }
    
    client.shutdown();
}

int main(int argc, char **argv) {
    switch (argc) {
        case 2: {
            servermode(std::uint16_t(atoi(argv[1])));
            break;
        }
        case 3:{
            clientmode(argv[1], std::uint16_t(atoi(argv[2])));
            break;
        }
        default:
            std::cout << "Usage:" << std::endl
                      << "\tdemo hostname port\t(start client)" << std::endl
                      << "\tdemo port\t\t(start server)" << std::endl;
    }
}


