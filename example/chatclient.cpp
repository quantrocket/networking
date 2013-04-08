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

void client_handler(ChatClient* client) {
    client->handle();
}

ChatClient::ChatClient(const std::string& ip, std::uint16_t port)
    : networking::Client() {
    this->authed = false;
    this->connect(ip, port);
    this->handler.start(client_handler, this);
    std::cout << "Client started" << std::endl;
}

ChatClient::~ChatClient() {
    this->link.close();
    this->handler.wait();
}

void ChatClient::handle() {
    while (this->isOnline()) {
        // wait for next object
        json::Value object = this->pop();
        if (!object.isNull()) {
            json::Value payload = object["payload"];
            std::string event = payload["event"].getString();

            if (event == "LOGIN_RESPONSE") {
                this->login(payload);
            } else if (event == "LOGOUT_RESPONSE") {
                this->logout(payload);
            } else if (event == "MESSAGE_RESPONSE") {
                this->message(payload);
            } else if (event == "USERLIST_UPDATE") {
                this->update(payload);
            }
        } else {
            networking::delay(15);
        }
    }
}

void ChatClient::login(json::Value data) {
    ClientID id = data["id"].getInteger();
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

void ChatClient::message(json::Value data) {
    ClientID id = data["id"].getInteger();
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

void ChatClient::logout(json::Value data) {
    ClientID id = data["id"].getInteger();
    if (this->authed && id == this->id) {
        std::cout << "You are leaving the chat." << std::endl;
        this->authed = false;
        this->username = "";
        this->users.clear();
        this->link.close();
    }
}

void ChatClient::update(json::Value data) {
    bool add = data["add"].getBoolean();
    ClientID id = data["id"].getInteger();
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


