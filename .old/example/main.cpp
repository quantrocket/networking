/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include <iostream>

#include "events.hpp"
#include "chatclient.hpp"
#include "chatserver.hpp"

int main(int argc, char **argv) {
    std::string input;
    std::string rest;

    ChatServer* server = NULL;
    ChatClient* client = NULL;

    if (SDL_Init(0) == -1) {
        std::cerr << "SDL_Init: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (SDLNet_Init() == -1) {
        std::cerr << "SDLNet_Init: " << SDLNet_GetError() << std::endl;
        return 1;
    }

    switch (argc) {
        case 2:
            server = new ChatServer((unsigned short)(atoi(argv[1])));
            while (true) {
                getline(std::cin, input);
                if (input == "quit") {
                    server->push(new LogoutRequest());
                    break;
                }
            }
            delete server;
            break;
        case 3:
            std::cout << "Username: ";
            getline(std::cin, input);
            client = new ChatClient(argv[1], (unsigned short)(atoi(argv[2])));
            client->push(new LoginRequest(input));
            while (client->isOnline()) {
                getline(std::cin, input);
                if (!client->authed) { continue; }
                if (input == "quit") {
                    client->push(new LogoutRequest());
                } else {
                    client->push(new MessageRequest(input));
                }
            }
            delete client;
            break;
        default:
            std::cout << "Usage:" << std::endl
                      << "\tdemo hostname port\t(start client)" << std::endl
                      << "\tdemo port\t\t(start server)" << std::endl;
    }

    SDLNet_Quit();
    SDL_Quit();
}


