#include "client.hpp"

#include <iostream>
#include <string.h>
#include <SDL/SDL.h>

// walkaround:
int client_thread(void* param) {
    Client* client = (Client*)param;
    client->handle();
}

Client::Client(EventPipe* in, EventPipe* out) {
    this->in = in;
    this->out = out;
    this->authed = false;
    this->running = true;
    // walkaround:
    this->thread = SDL_CreateThread(client_thread, (void*)this);
}

Client::~Client() {
    int* tmp;
    SDL_WaitThread(this->thread, tmp);
}

void Client::login(LoginResponse* data) {
    if (!this->authed) {
        this->authed = data->success;
    }
}

void Client::message(MessageResponse* data) {
    if (this->authed && data->success) {
        std::cout << data->text;
    }
}

void Client::logout(LogoutResponse* data) {
    if (this->authed) {
        this->authed = !data->success;
        this->running = !data->success;
        // note: input at demo.cpp might run once again :P 
    }
}

void Client::handle() {
    while (this->running) {
        Event* got = this->in->pop();
        if (got == NULL) {
            continue;
        }
        // Handle Response
        switch (got->event_id) {
            case eventid::LOGIN_RP:
                this->login((LoginResponse*)got);
                break;
            case eventid::MESSAGE_RP:
                this->message((MessageResponse*)got);
                break;
            case eventid::LOGOUT_RP:
                this->logout((LogoutResponse*)got);
                break;
        }
    }
}

void Client::do_login(std::string username) {
    LoginRequest* data = new LoginRequest();
    memcpy(data->username, username.c_str(), 255);
    this->out->push(data);
}

void Client::do_message(std::string text) {
    MessageRequest* data = new MessageRequest();
    memcpy(data->text, text.c_str(), 255);
    this->out->push(data);
}

void Client::do_logout() {
    LogoutRequest* data = new LogoutRequest();
    this->out->push(data);
}



