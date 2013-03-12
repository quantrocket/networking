/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include <iostream>
#include <string.h>

#include "src/connection.hpp"
#include "src/eventsystem.hpp"

// event id for the specialized event "Test" (see below)
const EventID TEST = 1;

// a specialized event
class Test: public Event {
    public:
        char text[255];
        Test()
            : Event(TEST) {
        }
        Test(Test* other)
            : Event(TEST) {
            strncpy(this->text, other->text, 255);
        }
        Test(std::string text)
            : Event(TEST) {
            strncpy(this->text, text.c_str(), 255);
        }
};

Event* Event::assemble(void* buffer) {
    Event* event = reinterpret_cast<Event*>(buffer);
    EventID id = event->event_id;
    switch (id) {
        case TEST:
            event = new Test((Test*)buffer);
            break;
    };
    return event;
}


void tcp_demo() {
    std::cout << "== TCP-based demo ==" << std::endl;
    
    TcpListener* server  = new TcpListener();
    TcpLink*     client  = new TcpLink();
    TcpLink*     service = NULL;
    
    // server socket listening
    server->open(14000);
    // then the client can connect
    client->open("localhost", 14000);
    // and the server can accept the client (and create a service link for him)
    service = server->accept();
    
    // the server can establish the event system using the socket to the client
    NetworkingQueue* serversided = new NetworkingQueue(service);
    // and the client can establish the event system using the socket to the server
    NetworkingQueue* clientsided = new NetworkingQueue(client);
    
    // the client can send a specialized event
    clientsided->push(new Test("Hello World :)"));

    // and the server can read the event
    Event* a = NULL;
    while (a == NULL) {
        a = serversided->pop();
    }
    // and handle it referring to it's actual kind
    if (a->event_id == TEST) {
        Test* data = static_cast<Test*>(a);
        std::cout << "The client sent via tcp: " << data->text << std::endl;
        delete a;
    }
    
    // now the server could answer
    serversided->push(new Test("How are you?"));
    
    // and the client is able to read the event
    a = NULL;
    while (a == NULL) {
        a = clientsided->pop();
    }
    // and handle it referrint to it's actual kind
    if (a->event_id == TEST) {
        Test* data = static_cast<Test*>(a);
        std::cout << "The server send via tcp: " << data->text << std::endl;
        delete a;
    }
    
    // cleaning up
    delete clientsided;
    delete serversided;
    // links closed automatically ;) 
    delete client;
    delete service;
    delete server;
}

/*
void udp_demo() {
    std::cout << "== UDP-based demo ==" << std::endl;
    
    UdpLink* server = new UdpLink();
    UdpLink* client = new UdpLink();
    
    // server listening on port 14000
    server->open(14000);
    // client listening on port 14001
    // (cannot be 14000 in this case, because both are running on localhost
    // otherwise, a 'NetworkError' with
    // 'SDLNet_UDP_Open: Couldn't bind to local port' is thrown
    client->open(14001);
    
    // the server should always send to localhost::14001 (= client)
    server->setTarget("localhost", 14001);
    // and the client should always send to localhost:14000 (= server)
    client->setTarget("localhost", 14000);
    // This can be changed when ever it is necessary. So one server can handle
    // multiple clients and changing sending target all the time. Otherwise
    // there can be dedicated services, each handling one client.
    
    // the server can establish the event system using the socket to the client
    NetworkingQueue* serversided = new NetworkingQueue(server);
    // and the client can establish the event system using the socket to the server
    NetworkingQueue* clientsided = new NetworkingQueue(client);
    
    // Because the NetworkingQueue gets a pointer, we can change our sending
    // target after creating the queue to force the queue sending to another
    // destination.
    
    // the client can send a specialized event - as before
    Test* a = new Test("Hello world :)");;
    clientsided->push(a);

    // and the server can read the event
    Event* b = NULL;
    while (b == NULL) {
        b = serversided->pop();
    }
    // and handle it referring to it's actual kind
    if (b->event_id == TEST) {
        Test* data = static_cast<Test*>(b);
        std::cout << "The client sent via udp: " << data->text << std::endl;
    }
    
    // now the server could answer
    Test* c = new Test("How are you?");
    serversided->push(c);
    
    // and the client is able to read the event
    Event* d = NULL;
    while (d == NULL) {
        d = clientsided->pop();
    }
    // and handle it referrint to it's actual kind
    if (d->event_id == TEST) {
        Test* data = static_cast<Test*>(d);
        std::cout << "The server send via udp: " << data->text << std::endl;
    }
    
    // cleaning up
    delete clientsided;
    delete serversided;
    // links closed automatically ;) 
    delete client;
    delete server;
}
*/

int main() {
    tcp_demo();
    //udp_demo();
}


