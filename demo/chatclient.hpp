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

#ifndef CHATCLIENT_HPP
#define CHATCLIENT_HPP

#include <iostream>
#include <map>

#include "../net/client.hpp"

class ChatClient;

class ChatClient: public net::Client<ChatClient> {

    protected:
        std::map<net::ClientID, std::string> users;

        void login(json::Var& data);
        void message(json::Var& data);
        void logout(json::Var& data);
        void update(json::Var& data);

        void fallback(json::Var& data);

    public:
        ChatClient(std::string const & ip, std::uint16_t const port);
        virtual ~ChatClient();

        bool authed;
        std::string username;

        void request_login(std::string const & username);
        void request_logout();
        void request_message(std::string const & message);

};

#endif

