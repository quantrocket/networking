/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "server.hpp"

#include <iostream>
#include <string>
#include <SDL/SDL.h>

// walkaround:
int server_thread(void* param) {
    Server* server = (Server*)param;
    server->handle();
}

Server::Server(EventPipe* in, EventPipe* out) {
    this->in = in;
    this->out = out;
    this->running = true;
    this->username = "";
    this->history = "";
    // walkaround:
    this->thread = SDL_CreateThread(server_thread, (void*)this);
}

Server::~Server() {
    int* tmp;
    SDL_WaitThread(this->thread, tmp);
}

void Server::login(LoginRequest* data) {
    LoginResponse* answer = new LoginResponse();
    
    if (data->username == "") {
        answer->success = false;
    } else {
        this->username = data->username;
        answer->success = true;
        std::cout << "IN  : " << data->username << std::endl;
    }
    // send answer to client
    out->push(answer);
}

void Server::message(MessageRequest* data) {
    MessageResponse* answer = new MessageResponse();
    
    if (data->text == "") {
        answer->success = false;
    } else {
        answer->success = true;
        std::string line = ">>  : " + std::string(data->text) + "\n";
        this->history += line;
        memcpy(answer->text, line.c_str(), 20000);
    }
    // send answer to client
    out->push(answer);
}

void Server::logout(LogoutRequest* data) {
    LogoutResponse* answer = new LogoutResponse();

    if (this->username.empty()) {
        answer->success = false;
    } else {
        std::cout << "OUT : " << this->username << std::endl;
        this->username = "";
        answer->success = true;
        this->running = false;
    }
    // send answer to client
    out->push(answer);
}

void Server::handle() {
    Event* got = NULL;
    while (this->running) {
        got = this->in->pop();
        if (got == NULL) {
            continue;
        }
        // Handle Requests
        switch (got->event_id) {
            case eventid::LOGIN_RQ:
                this->login((LoginRequest*)got);
                break;
            case eventid::MESSAGE_RQ:
                this->message((MessageRequest*)got);
                break;
            case eventid::LOGOUT_RQ:
                this->logout((LogoutRequest*)got);
                break;
        }
    }
}


