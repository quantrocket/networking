/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "common.hpp"

networking::Event* networking::Event::assemble(void* buffer) {
    networking::Event* event = reinterpret_cast<networking::Event*>(buffer);
    networking::EventID id = event->event_id;
    switch (id) {
        case E_LOGIN_REQUEST:
            event = new LoginRequest((LoginRequest*)buffer);
            break;
        case E_LOGIN_RESPONSE:
            event = new LoginResponse((LoginResponse*)buffer);
            break;
        case E_MESSAGE_REQUEST:
            event = new MessageRequest((MessageRequest*)buffer);
            break;
        case E_MESSAGE_RESPONSE:
            event = new MessageResponse((MessageResponse*)buffer);
            break;
        case E_LOGOUT_REQUEST:
            event = new LogoutRequest((LogoutRequest*)buffer);
            break;
        case E_LOGOUT_RESPONSE:
            event = new LogoutResponse((LogoutResponse*)buffer);
            break;
        case E_USERLIST_UPDATE:
            event = new UserlistUpdate((UserlistUpdate*)buffer);
    };
    return event;
}


