/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <cstdint>
#include <string.h>

#include "../src/eventsystem.hpp"
#include "../src/connection.hpp"
#include "../src/serverclient.hpp"

using networking::EventID;
using networking::Event;
using networking::ClientID;

const EventID E_LOGIN_REQUEST    = 1;
const EventID E_LOGIN_RESPONSE   = 2;
const EventID E_MESSAGE_REQUEST  = 3;
const EventID E_MESSAGE_RESPONSE = 4;
const EventID E_LOGOUT_REQUEST   = 5;
const EventID E_LOGOUT_RESPONSE  = 6;
const EventID E_USERLIST_UPDATE  = 7;

struct LoginRequest: public Event {
    char username[255];

    LoginRequest(const std::string& username) : Event(E_LOGIN_REQUEST) {
        strncpy(this->username, username.c_str(), 255);
    }

    LoginRequest(LoginRequest* other) : Event(E_LOGIN_REQUEST) {
        strncpy(this->username, other->username, 255);
    }
};

struct LoginResponse: public Event {
    bool success;
    ClientID id;
    char username[255];

    LoginResponse(bool success, ClientID id, const std::string& username)
        : Event(E_LOGIN_RESPONSE) {
        this->success = success;
        this->id = id;
        strncpy(this->username, username.c_str(), 255);
    }

    LoginResponse(LoginResponse* other) : Event(E_LOGIN_RESPONSE) {
        this->success = other->success;
        this->id      = other->id;
        strncpy(this->username, other->username, 255);
    }
};

struct MessageRequest: public Event {
    char text[20000];

    MessageRequest(const std::string& text) : Event(E_MESSAGE_REQUEST) {
        strncpy(this->text, text.c_str(), 20000);
    }

    MessageRequest(MessageRequest* other) : Event(E_MESSAGE_REQUEST) {
        strncpy(this->text, other->text, 20000);
    }
};

struct MessageResponse: public Event {
    char text[20000];
    ClientID id;

    MessageResponse(const std::string& text, ClientID id)
        : Event(E_MESSAGE_RESPONSE) {
        strncpy(this->text, text.c_str(), 20000);
        this->id = id;
    }

    MessageResponse(MessageResponse* other) : Event(E_MESSAGE_RESPONSE) {
        strncpy(this->text, other->text, 20000);
        this->id = other->id;
    }
};

struct LogoutRequest: public Event {
    LogoutRequest() : Event(E_LOGOUT_REQUEST) {
    }

    LogoutRequest(LogoutRequest* other) : Event(E_LOGOUT_REQUEST) {
    }
};

struct LogoutResponse: public Event {
    ClientID id;

    LogoutResponse(ClientID id) : Event(E_LOGOUT_RESPONSE) {
        this->id = id;
    }
    LogoutResponse(LogoutResponse* other) : Event(E_LOGOUT_RESPONSE) {
        this->id = other->id;
    }
};

struct UserlistUpdate: public Event {
    bool add; // false == remove
    ClientID id;
    char username[255];

    UserlistUpdate(bool add, ClientID id, const std::string& username)
        : Event(E_USERLIST_UPDATE) {
        this->add = add;
        this->id  = id;
        strncpy(this->username, username.c_str(), 255);
    }

    UserlistUpdate(UserlistUpdate* other) : Event(E_USERLIST_UPDATE) {
        this->add = other->add;
        this->id  = other->id;
        strncpy(this->username, other->username, 255);
    }
};

#endif

