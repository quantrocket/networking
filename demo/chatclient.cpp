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

#include "chatclient.hpp"
#include "commands.hpp"

ChatClient::ChatClient(const std::string& ip, std::uint16_t port)
    : net::Client<ChatClient>() {

    this->callbacks[commands::LOGIN_RESPONSE]   = &ChatClient::login;
    this->callbacks[commands::LOGOUT_RESPONSE]  = &ChatClient::logout;
    this->callbacks[commands::MESSAGE_RESPONSE] = &ChatClient::message;
    this->callbacks[commands::USERLIST_UPDATE]  = &ChatClient::update;

    this->authed = false;
    this->connect(ip, port);
    std::cout << "Client started" << std::endl;
}

ChatClient::~ChatClient() {
    this->link.close();
}

void ChatClient::login(json::Var data) {
    net::ClientID id = data["id"].getInteger();
    std::string username = data["username"].getString();
    bool success = data["success"].getBoolean();
    if (!this->authed && id == this->id) {
        if (success) {
            this->username = username;
            this->authed = success;
            this->users[id] = username;
            std::cout << "You entered the chat as '" << username << "'"
                      << std::endl;
        }
    }
}

void ChatClient::message(json::Var data) {
    net::ClientID id = data["id"].getInteger();
    std::string text = data["text"].getString();
    if (this->authed) {
        auto node = this->users.find(id);
        if (node == this->users.end()) {
            // not found (ignore event)
            return;
        }
        std::cout << "<" << (node->second) << "> " << text << std::endl;
    }
}

void ChatClient::logout(json::Var data) {
    net::ClientID id = data["id"].getInteger();
    if (this->authed && id == this->id) {
        std::cout << "You are leaving the chat." << std::endl;
        this->authed = false;
        this->username = "";
        this->users.clear();
        this->link.close();
    }
}

void ChatClient::update(json::Var data) {
    bool add = data["add"].getBoolean();
    net::ClientID id = data["id"].getInteger();
    std::string username = data["username"].getString();
    if (add) {
        // add to userlist
        this->users[id] = username;
        std::cout << "'" << username << "' was added to the userlist."
                  << std::endl;
    } else {
        // remove from userlist
        auto node = this->users.find(id);
        if (node == this->users.end()) {
            return; // not found (ignored)
        }
        std::cout << "'" << (node->second) << "' was remove from the userlist."
                  << std::endl;
        this->users.erase(node);
    }
}

void ChatClient::request_login(const std::string& username) {
    json::Var request;
    request["command"] = commands::LOGIN_REQUEST;
    request["username"] = username;
    this->push(request);
}

void ChatClient::request_logout() {
    json::Var request;
    request["command"] = commands::LOGOUT_REQUEST;
    this->push(request);
}

void ChatClient::request_message(const std::string& message) {
    json::Var request;
    request["command"] = commands::MESSAGE_REQUEST;
    request["text"] = message;
    this->push(request);
}

