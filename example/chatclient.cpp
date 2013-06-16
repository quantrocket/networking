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

#include "chatclient.hpp"

ChatClient::ChatClient(std::string const & ip, std::uint16_t const port)
    : net::Client<ChatProtocol>() {
    // Assign Callback Methods
    this->attach(commands::LOGIN_RESPONSE, &ChatClient::login);
    this->attach(commands::LOGOUT_RESPONSE, &ChatClient::logout);
    this->attach(commands::MESSAGE_RESPONSE, &ChatClient::message);
    this->attach(commands::USERLIST_UPDATE, &ChatClient::update);

    this->authed = false;
    if (this->connect(ip, port)) {
        std::cout << "Client started" << std::endl;
    }
}

ChatClient::~ChatClient() {
    this->link.disconnect();
}

void ChatClient::login(ChatProtocol & data) {
    if (!this->authed && data.userid == this->id && data.success) {
        this->username           = data.username;
        this->authed             = true;
        this->users[data.userid] = data.username;
        std::cout << "You entered the chat as '" << data.username << "'"
                  << std::endl;
    }
}

void ChatClient::message(ChatProtocol & data) {
    if (this->authed) {
        auto node = this->users.find(data.userid);
        if (node == this->users.end()) {
            // author not found
            return;
        }
        std::cout << "<" << (node->second) << "> " << data.text << std::endl;
    }
}

void ChatClient::logout(ChatProtocol & data) {
    if (this->authed && data.userid == this->id) {
        std::cout << "You are leaving the chat." << std::endl;
        this->authed = false;
        this->username = "";
        this->users.clear();
        this->link.disconnect();
    }
}

void ChatClient::update(ChatProtocol & data) {
    if (data.add_user) {
        // add to userlist
        this->users[data.userid] = data.username;
        std::cout << "'" << data.username << "' was added to the userlist."
                  << std::endl;
    } else {
        // remove from userlist
        auto node = this->users.find(data.userid);
        if (node == this->users.end()) {
            return; // not found (ignored)
        }
        std::cout << "'" << (node->second) << "' was removed from the userlist."
                  << std::endl;
        this->users.erase(node);
    }
}

void ChatClient::fallback(ChatProtocol & data) {
    std::cout << "Unknown command #" << data.command << std::endl;
}

void ChatClient::request_login(std::string const & username) {
    ChatProtocol request;
    request.command  = commands::LOGIN_REQUEST;
    request.username = username;
    this->push(request);
}

void ChatClient::request_logout() {
    ChatProtocol request;
    request.command = commands::LOGOUT_REQUEST;
    this->push(request);
}

void ChatClient::request_message(std::string const & message) {
    ChatProtocol request;
    request.command = commands::MESSAGE_REQUEST;
    request.text = message;
    this->push(request);
}

