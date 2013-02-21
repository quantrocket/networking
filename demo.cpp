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

namespace chat {
    bool authed = false; // describes, whether user is authed or not
    std::string username;

    void login(Login* data) {
        std::cout << "IN  : " << data->username << "!" << std::endl;
        username = data->username;
        authed = true;
    }
    void message(Message* data) {
        std::cout << ">>  : '" << data->text << "'" << std::endl;
    }
    void logout(Logout* data) {
        std::cout << "OUT : " << data->username << "!" << std::endl;
        username = "";
        authed = false;
    }
}

int main() {
    EventPipe pipe;
    bool running = true;
    Event* got = NULL;

    while (running) {
        // Handle Input Stuff
        std::string input;
        std::cin >> input;
        if (!chat::authed) {
            // handle input as login
            Login* data = new Login();
            memcpy(data->username, input.c_str(), 255);
            pipe.push(data);
        } else {
            if (input == "/q") {
                // handle input as logout
                Logout* data = new Logout();
                memcpy(data->username, chat::username.c_str(), 255);
                pipe.push(data);
            } else {
                // handle input as message
                Message* data = new Message();
                memcpy(data->text, input.c_str(), 20000);
                pipe.push(data);
            }
        }
        // Handle Events
        got = pipe.pop();
        if (got == NULL) {
            continue;
        }
        switch (got->event_id) {
            case eventid::LOGIN:
                chat::login((Login*)got);
                break;
            case eventid::MESSAGE:
                chat::message((Message*)got);
                break;
            case eventid::LOGOUT:
                chat::logout((Logout*)got);
                running = false;
                break;
        }
    }
}



