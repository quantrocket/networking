/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include <iostream>

#include "src/connection.hpp"
#include "src/eventsystem.hpp"

// event id for the specialized event "Test" (see below)
const EventID TEST = 1;

// a specialized event
struct Test: Event {
    char text[255];
    Test(): Event(TEST) {}
};

int main() {
    TcpConnection* server = new TcpConnection();
    TcpConnection* client = new TcpConnection();
    TcpConnection* service = NULL;

    // server socket listening
    server->listen(14000);
    // then the client can connect
    client->connect("localhost", 14000);
    // and the server can accept the client
    // (and create a service socket handling him)
    service = server->accept();
    
    // the server can establish the event system using the socket to the client
    EventSystem<TcpConnection>* serversided = new EventSystem<TcpConnection>(service);
    // and the client can establish the event system using the socket to the server
    EventSystem<TcpConnection>* clientsided = new EventSystem<TcpConnection>(client);
    
    // the client can send a specialized event
    Test* a = new Test();
    memcpy(a->text, "Hello world :)", 14);
    clientsided->push(a);

    // and the server can read the event
    Event* b = NULL;
    while (b == NULL) {
        b = serversided->pop();
    }
    // and handle it referring to it's actual kind
    if (b->event_id == TEST) {
        Test* data = (Test*)b;
        std::cout << data->text << std::endl;
    }

}


