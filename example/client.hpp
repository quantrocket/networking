/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "common.hpp"

// Client class
class Client {
    protected:
        EventPipe* in;
        EventPipe* out;
        SDL_Thread* thread;
        
        void login(LoginResponse* data);
        void message(MessageResponse* data);
        void logout(LogoutResponse* data);
        
    public:
        bool running;
        bool authed;

        Client(EventPipe* in, EventPipe* out);
        ~Client();

        void handle();
        void do_login(std::string username);
        void do_message(std::string text);
        void do_logout();
};

int client_thread(void* param);

#endif
