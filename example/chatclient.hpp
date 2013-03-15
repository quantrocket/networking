/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef CHATCLIENT_HPP
#define CHATCLIENT_HPP

#include <iostream>
#include <map>

#include "common.hpp"

int client_handler(void* param);

class ChatClient: public Client {
    friend int client_handler(void* param);
    protected:
        Thread handler;

        std::map<ClientID, std::string> users;

        void handle();
        void login(LoginResponse* data);
        void message(MessageResponse* data);
        void logout(LogoutResponse* data);
        void update(UserlistUpdate* data);
    public:
        ChatClient(const std::string& ip, unsigned short port);
        virtual ~ChatClient();
        
        bool authed;
        ClientID id;
        std::string username; 
};

#endif

