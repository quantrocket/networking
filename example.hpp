/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#ifndef EXAMPLE_HPP
#define EXAMPLE_HPP

#include "src/eventsystem.hpp"

#include <cstdint>
#include <string.h>
#include <string>

using networking::EventID;
using networking::Event;
using networking::ClientID;

const EventID USERLISTUPDATE	= 1;
const EventID LOGINREQUEST	= 2;
const EventID MESSAGERESPONSE	= 3;
const EventID LOGOUTREQUEST	= 4;
const EventID MESSAGEREQUEST	= 5;
const EventID LOGOUTRESPONSE	= 6;
const EventID LOGINRESPONSE	= 7;

struct UserlistUpdate: public Event {
    char username[255];
    bool add;
    ClientID id;

    UserlistUpdate(const std::string& username, bool add, ClientID id)
        : Event(E_USERLISTUPDATE) {
        strncpy(this->username, username, 255);
        this->add = add;
        this->id = id;
    }
};

struct LoginRequest: public Event {
    char username[255];

    LoginRequest(const std::string& username)
        : Event(E_LOGINREQUEST) {
        strncpy(this->username, username, 255);
    }
};

struct MessageResponse: public Event {
    char text[20000];
    ClientID id;

    MessageResponse(const std::string& text, ClientID id)
        : Event(E_MESSAGERESPONSE) {
        strncpy(this->text, text, 20000);
        this->id = id;
    }
};

struct LogoutRequest: public Event {
    LogoutRequest()
        : Event(E_LOGOUTREQUEST) {
    }
};

struct MessageRequest: public Event {
    char text[20000];

    MessageRequest(const std::string& text)
        : Event(E_MESSAGEREQUEST) {
        strncpy(this->text, text, 20000);
    }
};

struct LogoutResponse: public Event {
    ClientID id;

    LogoutResponse(ClientID id)
        : Event(E_LOGOUTRESPONSE) {
        this->id = id;
    }
};

struct LoginResponse: public Event {
    char username[255];
    ClientID id;
    bool success;

    LoginResponse(const std::string& username, ClientID id, bool success)
        : Event(E_LOGINRESPONSE) {
        strncpy(this->username, username, 255);
        this->id = id;
        this->success = success;
    }
};


Event* Event::assemble(void* buffer) {
    Event* event = reinterpret_cast<Event*>(buffer);
    EventID id = event->event_id;
    switch (id) { 
        case E_USERLISTUPDATE:
            event = new UserlistUpdate(*(UserlistUpdate*)buffer);
            break; 
        case E_LOGINREQUEST:
            event = new LoginRequest(*(LoginRequest*)buffer);
            break; 
        case E_MESSAGERESPONSE:
            event = new MessageResponse(*(MessageResponse*)buffer);
            break; 
        case E_LOGOUTREQUEST:
            event = new LogoutRequest(*(LogoutRequest*)buffer);
            break; 
        case E_MESSAGEREQUEST:
            event = new MessageRequest(*(MessageRequest*)buffer);
            break; 
        case E_LOGOUTRESPONSE:
            event = new LogoutResponse(*(LogoutResponse*)buffer);
            break; 
        case E_LOGINRESPONSE:
            event = new LoginResponse(*(LoginResponse*)buffer);
            break;};
    return event;
}

#endif
