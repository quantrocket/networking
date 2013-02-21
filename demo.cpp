#include <iostream>
#include <queue>
#include <string.h>

#include "src/event.hpp"

// separate namespace to keep global namespace clear
namespace eventid {
    extern const EventID LOGIN   = 1;
    extern const EventID MESSAGE = 2;
    extern const EventID LOGOUT  = 3;
}

struct Login: Event {
    // usefull for sending via TCP/UDP (later)
    char username[255];
    // make this event a LOGIN-event
    Login(): Event(eventid::LOGIN) {}
};

struct Message: Event {
    char text[20000];
    Message(): Event(eventid::MESSAGE) {}
};

struct Logout: Event {
    char username[255];
    Logout(): Event(eventid::LOGOUT) {}
};

void handle_login(Login* data) {
    std::cout << "IN  : " << data->username << "!" << std::endl;
}

void handle_message(Message* data) {
    std::cout << ">>  : '" << data->text << "'" << std::endl;
}

void handle_logout(Logout* data) {
    std::cout << "OUT : " << data->username << "!" << std::endl;
}

int main() {
    EventPipe pipe;
    // sending stuff
    Login* login = new Login();
    memcpy(login->username, "Tester", 7);
    pipe.push(login);
    // note: login should not be deleted here!
    Message* message = new Message();
    memcpy(message->text, "Hallo World & Good Night ;D", 28);
    pipe.push(message);
    Logout* logout = new Logout();
    memcpy(logout->username, "Tester", 7);
    pipe.push(logout);
    
    // receive
    Event* got = NULL;
    bool quit = false;
    while (quit == false) {
        got = pipe.pop();
        if (got == NULL) {
            // skip blank event
            continue;
        }
        switch (got->event_id) {
            case eventid::LOGIN:
                // handle event
                handle_login((Login*)got);
                // destroy event
                delete got;
                break;
            case eventid::MESSAGE:
                handle_message((Message*)got);
                delete got;
                break;
            case eventid::LOGOUT:
                handle_logout((Logout*)got);
                delete got;
                quit = true;
                break;
        }
    }
}



