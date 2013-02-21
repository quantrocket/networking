/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef SERVER_HPP
#define SERVER_HPP

#include "common.hpp"

// Server class
class Server {
    protected:
        EventPipe* in;
        EventPipe* out;
        bool running;
        std::string username;
        std::string history;
        SDL_Thread* thread;
        
        void login(LoginRequest* data);
        void message(MessageRequest* data);
        void logout(LogoutRequest* data);
        
    public:
        Server(EventPipe* in, EventPipe* out);
        ~Server();

        void handle();
};

int server_thread(void* param);

#endif
