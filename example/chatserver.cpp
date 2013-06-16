/*
Copyright (c) 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers a tcp-based server-client framework for games and other software.

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

ChatServer::ChatServer(const std::uint16_t port)
    : net::Server<ChatProtocol>(5) {
    // Assign Callback Methods
    this->attach(commands::LOGIN_REQUEST, &ChatServer::login);
    this->attach(commands::LOGOUT_REQUEST, &ChatServer::logout);
    this->attach(commands::MESSAGE_REQUEST, &ChatServer::message);

    if (this->start(port)) {
        std::cout << "Server started" << std::endl;
    }
    this->block("127.0.0.1");
}

ChatServer::~ChatServer() {
    this->listener.close();
    std::cout << "Server stopped" << std::endl;
}

void ChatServer::login(ChatProtocol & data) {
    // seek id
    auto node = this->users.find(data.client);
    if (node != this->users.end()) {
        // loggin failed (user already logged in)
        ChatProtocol answer;
        answer.command = commands::LOGIN_RESPONSE;
        answer.success = false;
        this->push(answer, data.client);
        return;
    }
    // add user
    this->users[data.client] = data.username;
    this->group(data.client, 0); // add to group #0
    // loggin successful
    ChatProtocol answer;
    answer.command  = commands::LOGIN_RESPONSE;
    answer.success  = true;
    answer.userid   = data.client;
    answer.username = data.username;
    this->push(answer, data.client);
    node = this->users.begin();
    while (node != this->users.end()) {
        if (node->first != data.client) {
            // send all user ids & names to this client
            ChatProtocol answer;
            answer.command  = commands::USERLIST_UPDATE;
            answer.add_user = true;
            answer.userid   = node->first;
            answer.username = node->second;
            this->push(answer, data.client);
            // send this new user to all other clients
            answer.userid   = data.client;
            answer.username = data.username;
            this->push(answer, node->first);
        }
        node++;
    }
}

void ChatServer::message(ChatProtocol & data) {
    // seek id
    auto node = this->users.find(data.client);
    if (node == this->users.end()) {
        // user not logged in (ignore event)
        return;
    }
    // show message
    std::cout << "<" << (node->second) << "> " << data.text << std::endl;

    // send to all clients in the given group
    ChatProtocol answer;
    answer.command = commands::MESSAGE_RESPONSE;
    answer.text    = data.text;
    answer.userid  = data.client;
    this->pushGroup(answer, 0); // to group #0
}

void ChatServer::logout(ChatProtocol & data) {
    // seek id
    auto node = this->users.find(data.client);
    if (node == this->users.end()) {
        // user not logged in (ignore event);
        return;
    }
    std::string username = node->second;
    this->users.erase(node);
    // logout successful
    ChatProtocol answer;
    answer.command = commands::LOGOUT_RESPONSE;
    answer.userid  = data.client;
    this->push(answer, data.client);
    node = this->users.begin();
    while (node != this->users.end()) {
        if (node->first != data.client) {
            // send this new user to all other clients
            ChatProtocol answer;
            answer.command  = commands::USERLIST_UPDATE;
            answer.add_user = false;
            answer.userid   = data.client;
            answer.username = username;
            this->push(answer, node->first);
        }
        node++;
    }
    // user is automatically removed from all groups when disconnecting
}

void ChatServer::fallback(ChatProtocol & data) {
    std::cout << "Unknown command #" << data.command << std::endl;
}

void ChatServer::request_logout() {
    ChatProtocol request;
    request.command = commands::LOGOUT_REQUEST;
    this->push(request);
}

