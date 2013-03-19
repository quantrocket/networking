/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "chatserver.hpp"

void server_handler(ChatServer* server) {
    server->handle();
}

ChatServer::ChatServer(unsigned short port)
    : networking::Server() {
    this->start(port);
    this->handler.start(server_handler, this);
    std::cout << "Server started" << std::endl;
}

ChatServer::~ChatServer() {
    this->listener.close();
    this->handler.stop();
    std::cout << "Server stopped" << std::endl;
}

void ChatServer::handle() {
    while (this->isOnline()) {
        // wait for next bundle
        networking::Bundle* bundle = this->pop();
        if (bundle != NULL) {
            // handle bundle
            switch (bundle->event->event_id) {
                case E_LOGIN_REQUEST:
                    this->login((LoginRequest*)(bundle->event), bundle->id);
                    break;
                case E_LOGOUT_REQUEST:
                    this->logout((LogoutRequest*)(bundle->event), bundle->id);
                    break;
                case E_MESSAGE_REQUEST:
                    this->message((MessageRequest*)(bundle->event), bundle->id);
                    break;
            }
            delete bundle;
        } else {
            networking::delay(15);
        }
    }
}

void ChatServer::login(LoginRequest* data, ClientID id) {
    // seek id
    auto node = this->users.find(id);
    if (node != this->users.end()) {
        // loggin failed (user already logged in)
        this->push(new LoginResponse(false, 0, ""), id);
        return;
    }
    // add user
    this->users[id] = data->username;
    // loggin successful
    this->push(new LoginResponse(true, id, data->username), id);
    node = this->users.begin();
    while (node != this->users.end()) {
        if (node->first != id) {
            // send all user ids & names to this client
            this->push(new UserlistUpdate(true, node->first, node->second), id);
            // send this new user to all other clients
            this->push(new UserlistUpdate(true, id, data->username), node->first);
        }
        node++;
    }
}

void ChatServer::message(MessageRequest* data, ClientID id) {
    // seek id
    auto node = this->users.find(id);
    if (node == this->users.end()) {
        // user not logged in (ignore event)
        return;
    }
    // show message
    std::cout << "<" << (node->second) << "> " << data->text << std::endl;
    // broadcast message to clients
    node = this->users.begin();
    while (node != this->users.end()) {
        this->push(new MessageResponse(data->text, id), node->first);
        node++;
    }
}

void ChatServer::logout(LogoutRequest* data, ClientID id) {
    // seek id
    auto node = this->users.find(id);
    if (node == this->users.end()) {
        // user not logged in (ignore event);
        return;
    }
    std::string username = node->second;
    this->users.erase(node);
    // logout successful
    this->push(new LogoutResponse(id), id);
    node = this->users.begin();
    while (node != this->users.end()) {
        if (node->first != id) {
            // send this new user to all other clients
            this->push(new UserlistUpdate(false, id, username), node->first);
        }
        node++;
    }
}

