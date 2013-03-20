/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef EVENTS_HPP_GUARD
#define EVENTS_HPP_GUARD

#include "../src/eventsystem.hpp"
#include "../src/serverclient.hpp"

#include <cstdint>
#include <string.h>
#include <string>

using networking::EventID;
using networking::Event;
using networking::ClientID;

const EventID E_LOGIN_REQUEST = 1;
const EventID E_LOGIN_RESPONSE = 2;
const EventID E_MESSAGE_REQUEST = 3;
const EventID E_MESSAGE_RESPONSE = 4;
const EventID E_LOGOUT_REQUEST = 5;
const EventID E_LOGOUT_RESPONSE = 6;
const EventID E_USERLIST_UPDATE = 7;

struct LoginRequest: public Event {
    char username[255];

    LoginRequest(const std::string& username);
};

struct LoginResponse: public Event {
    bool success;
    ClientID id;
    char username[255];

    LoginResponse(bool success, ClientID id, const std::string& username);
};

struct MessageRequest: public Event {
    char text[20000];

    MessageRequest(const std::string& text);
};

struct MessageResponse: public Event {
    char text[20000];
    ClientID id;

    MessageResponse(const std::string& text, ClientID id);
};

struct LogoutRequest: public Event {
    LogoutRequest();
};

struct LogoutResponse: public Event {
    ClientID id;

    LogoutResponse(ClientID id);
};

struct UserlistUpdate: public Event {
    bool add;
    ClientID id;
    char username[255];

    UserlistUpdate(bool add, ClientID id, const std::string& username);
};


#endif
