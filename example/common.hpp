#ifndef COMMON_HPP
#define COMMON_HPP

#include "../src/event.hpp"

namespace eventid {
    const EventID LOGIN_RQ   = 1;
    const EventID MESSAGE_RQ = 2;
    const EventID LOGOUT_RQ  = 3;
    
    const EventID LOGIN_RP   = 4;
    const EventID MESSAGE_RP = 5;
    const EventID LOGOUT_RP  = 6;
}

struct LoginRequest: Event {
    // usefull for sending via TCP/UDP (later)
    char username[255];
    // make this event a LOGIN_RQ-event
    LoginRequest(): Event(eventid::LOGIN_RQ) {}
};

struct MessageRequest: Event {
    char text[20000];
    MessageRequest(): Event(eventid::MESSAGE_RQ) {}
};

struct LogoutRequest: Event {
    LogoutRequest(): Event(eventid::LOGOUT_RQ) {}
};   
    
struct LoginResponse: Event {
    bool success;
    LoginResponse(): Event(eventid::LOGIN_RP) {}
};

struct MessageResponse: Event {
    bool success;
    char text[20000];
    MessageResponse(): Event(eventid::MESSAGE_RP) {}
};

struct LogoutResponse: Event {
    bool success;
    LogoutResponse(): Event(eventid::LOGOUT_RP) {}
};
    
#endif
