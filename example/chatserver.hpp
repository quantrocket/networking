/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef CHATSERVER_HPP
#define CHATSERVER_HPP

#include <iostream>
#include <map>

#include "events.hpp"

#include "../src/connection.hpp"
#include "../src/serverclient.hpp"

class ChatServer;

void server_handler(ChatServer* server);

class ChatServer: public networking::Server {
    friend void server_handler(ChatServer* server);
    protected:
        networking::Thread handler;

        std::map<ClientID, std::string> users;

        void handle();
        void login(LoginRequest* data, ClientID id);
        void message(MessageRequest* data, ClientID id);
        void logout(LogoutRequest* data, ClientID id);
    public:
        ChatServer(unsigned short port);
        virtual ~ChatServer();
};

#endif

