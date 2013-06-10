/*
Copyright (c) 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers a json-based networking framework for games and other software.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "chatserver.hpp"
#include "commands.hpp"

ChatServer::ChatServer(const std::uint16_t port)
    : net::Server() {
    this->attach(commands::LOGIN_REQUEST, &ChatServer::login);
    this->attach(commands::LOGOUT_REQUEST, &ChatServer::logout);
    this->attach(commands::MESSAGE_REQUEST, &ChatServer::message);

    this->start(port);
    std::cout << "Server started" << std::endl;
}

ChatServer::~ChatServer() {
    this->listener.close();
    std::cout << "Server stopped" << std::endl;
}

 void ChatServer::login(json::Var & data, net::ClientID const id) {
    // seek id
    auto node = this->users.find(id);
    if (node != this->users.end()) {
        // loggin failed (user already logged in)
        json::Var answer;
        answer["command"] = commands::LOGIN_RESPONSE;
        answer["success"] = false;
        this->push(answer, id);
        return;
    }
    std::string username;
    if (!data["username"].get(username)) {
        return;
    }
    // add user
    this->users[id] = username;
    this->group(id, 0); // add to group #0
    // loggin successful
    json::Var answer;
    answer["command"] = commands::LOGIN_RESPONSE;
    answer["success"] = true;
    answer["id"] = id;
    answer["username"] = username;
    this->push(answer, id);
    node = this->users.begin();
    while (node != this->users.end()) {
        if (node->first != id) {
            // send all user ids & names to this client
            json::Var answer;
            answer["command"] = commands::USERLIST_UPDATE;
            answer["add"] = true;
            answer["id"] = node->first;
            answer["username"] = node->second;
            this->push(answer, id);
            // send this new user to all other clients
            answer["id"] = id;
            answer["username"] = username;
            this->push(answer, node->first);
        }
        node++;
    }
}

void ChatServer::message(json::Var & data, net::ClientID const id) {
    std::string text;
    if (!data["text"].get(text)) {
        return;
    }
    // seek id
    auto node = this->users.find(id);
    if (node == this->users.end()) {
        // user not logged in (ignore event)
        return;
    }
    // show message
    std::cout << "<" << (node->second) << "> " << text << std::endl;

    /// Method2 #1: iterate through all clients
    /*
    // broadcast message to clients
    node = this->users.begin();
    while (node != this->users.end()) {
        json::Var answer;
        answer["command"] = commands::MESSAGE_RESPONSE;
        answer["text"] = text;
        answer["id"] = id;
        this->push(answer, node->first);
        node++;
    }
    */

    /// Methode #2: send to all clients directly
    /*
    json::Var answer;
    answer["command"] = commands::MESSAGE_RESPONSE;
    answer["text"] = text;
    answer["id"] = id;
    this->push(answer);
    */

    /// Methode #3: send to all clients in the given group
    json::Var answer;
    answer["command"] = commands::MESSAGE_RESPONSE;
    answer["text"] = text;
    answer["id"] = id;
    this->pushGroup(answer, 0); // to group #0
}

void ChatServer::logout(json::Var& data, net::ClientID const id) {
    // seek id
    auto node = this->users.find(id);
    if (node == this->users.end()) {
        // user not logged in (ignore event);
        return;
    }
    std::string username = node->second;
    this->users.erase(node);
    // logout successful
    json::Var answer;
    answer["command"] = commands::LOGOUT_RESPONSE;
    answer["id"] = id;
    this->push(answer, id);
    node = this->users.begin();
    while (node != this->users.end()) {
        if (node->first != id) {
            // send this new user to all other clients
            json::Var answer;
            answer["command"] = commands::USERLIST_UPDATE;
            answer["add"] = false;
            answer["id"] = id;
            answer["username"] = username;
            this->push(answer, node->first);
        }
        node++;
    }
    // user is automatically removed from all groups when disconnecting
}

void ChatServer::fallback(json::Var& data, net::ClientID const id) {
    std::cout << "Unknown case from #" << id << " : " << data.dump() << std::endl;
}

void ChatServer::request_logout() {
    json::Var request;
    request["command"] = commands::LOGOUT_REQUEST;
    this->push(request);
}

