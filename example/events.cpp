/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "events.hpp"

LoginRequest::LoginRequest(const std::string& username)
    : Event(E_LOGIN_REQUEST) {
    strncpy(this->username, username.c_str(), 255);
}

LoginResponse::LoginResponse(bool success, ClientID id, const std::string& username)
    : Event(E_LOGIN_RESPONSE) {
    this->success = success;
    this->id = id;
    strncpy(this->username, username.c_str(), 255);
}

MessageRequest::MessageRequest(const std::string& text)
    : Event(E_MESSAGE_REQUEST) {
    strncpy(this->text, text.c_str(), 20000);
}

MessageResponse::MessageResponse(const std::string& text, ClientID id)
    : Event(E_MESSAGE_RESPONSE) {
    strncpy(this->text, text.c_str(), 20000);
    this->id = id;
}

LogoutRequest::LogoutRequest()
    : Event(E_LOGOUT_REQUEST) {
}

LogoutResponse::LogoutResponse(ClientID id)
    : Event(E_LOGOUT_RESPONSE) {
    this->id = id;
}

UserlistUpdate::UserlistUpdate(bool add, ClientID id, const std::string& username)
    : Event(E_USERLIST_UPDATE) {
    this->add = add;
    this->id = id;
    strncpy(this->username, username.c_str(), 255);
}


Event* Event::assemble(void* buffer) {
    Event* event = reinterpret_cast<Event*>(buffer);
    EventID id = event->event_id;
    switch (id) { 
        case E_LOGIN_REQUEST:
            event = new LoginRequest(*(LoginRequest*)buffer);
            break; 
        case E_LOGIN_RESPONSE:
            event = new LoginResponse(*(LoginResponse*)buffer);
            break; 
        case E_MESSAGE_REQUEST:
            event = new MessageRequest(*(MessageRequest*)buffer);
            break; 
        case E_MESSAGE_RESPONSE:
            event = new MessageResponse(*(MessageResponse*)buffer);
            break; 
        case E_LOGOUT_REQUEST:
            event = new LogoutRequest(*(LogoutRequest*)buffer);
            break; 
        case E_LOGOUT_RESPONSE:
            event = new LogoutResponse(*(LogoutResponse*)buffer);
            break; 
        case E_USERLIST_UPDATE:
            event = new UserlistUpdate(*(UserlistUpdate*)buffer);
            break;
    };
    return event;
}
