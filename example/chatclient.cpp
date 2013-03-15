/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

#include "chatclient.hpp"

int client_handler(void* param) {
    ChatClient* client = (ChatClient*)param;
    client->handle();
    return 0;
}

ChatClient::ChatClient(const std::string& ip, unsigned short port)
    : Client() {
    this->authed = false;
    this->connect(ip, port);
    this->handler.run(client_handler, (void*)this);
    std::cout << "Client started" << std::endl;
}

ChatClient::~ChatClient() {
    this->handler.wait();
    std::cout << "Client stopped" << std::endl;
}

void ChatClient::handle() {
    while (this->isOnline()) {
        // wait for next bundle
        ServerData* bundle = NULL;
        while (bundle == NULL) {
            SDL_Delay(50),
            bundle = this->pop();
        }
        // handle bundle
        switch (bundle->event->event_id) {
            case E_LOGIN_RESPONSE:
                this->login((LoginResponse*)(bundle->event));
                break;
            case E_MESSAGE_RESPONSE:
                this->message((MessageResponse*)(bundle->event));
                break;
            case E_LOGOUT_RESPONSE:
                this->logout((LogoutResponse*)(bundle->event));
                break;
            case E_USERLIST_UPDATE:
                this->update((UserlistUpdate*)(bundle->event));
        }
        // delete bundle
        delete bundle->event;
        delete bundle;
    }
}

void ChatClient::login(LoginResponse* data) {
    if (!this->authed) {
        if (data->success) {
            this->username = data->username;
            this->authed = data->success;
            this->id = data->id;
            this->users[data->id] = data->username;
            std::cout << "You entered the chat as '" << data->username << "'"
                      << std::endl;
        }
    }
}

void ChatClient::message(MessageResponse* data) {
    if (this->authed) {
        auto node = this->users.find(data->id);
        if (node == this->users.end()) {
            // not found (ignore event)
            return;
        }
        std::cout << "<" << (node->second) << "> " << data->text << std::endl;
    }
}

void ChatClient::logout(LogoutResponse* data) {
    if (this->authed && data->id == this->id) {
        std::cout << "You are leaving the chat." << std::endl;
        this->authed = false;
        this->id = 0;
        this->username = "";
        this->users.clear();
    }
}

void ChatClient::update(UserlistUpdate* data) {
    if (data->add) {
        // add to userlist
        this->users[data->id] = data->username;
        std::cout << "'" << (data->username) << "' was added to the userlist."
                  << std::endl;
    } else {
        // remove from userlist
        auto node = this->users.find(data->id);
        if (node == this->users.end()) {
            return; // not found (ignored)
        }
        std::cout << "'" << (node->second) << "' was remove from the userlist."
                  << std::endl;
        this->users.erase(node);
    }
}


