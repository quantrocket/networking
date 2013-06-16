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

#pragma once
#ifndef COMMANDS_HPP_INCLUDED
#define COMMANDS_HPP_INCLUDED

#include <iostream>

#include <SFML/Network.hpp>

#include <net/common.hpp>

namespace commands {

    const net::CommandID LOGIN_REQUEST    = 1;
    const net::CommandID LOGIN_RESPONSE   = 2;
    const net::CommandID LOGOUT_REQUEST   = 3;
    const net::CommandID LOGOUT_RESPONSE  = 4;
    const net::CommandID MESSAGE_REQUEST  = 5;
    const net::CommandID MESSAGE_RESPONSE = 6;
    const net::CommandID USERLIST_UPDATE  = 7;

}

class ChatProtocol: public net::BaseProtocol {

    public:
        bool send(sf::TcpSocket & socket) {
            // Pack data
            sf::Packet packet;
            packet << this->command;
            switch (this->command) {
            
                case commands::LOGIN_REQUEST:
                    packet << this->username;
                    break;
                    
                case commands::LOGIN_RESPONSE:
                    packet << this->username << this->userid << this->success;
                    break;
                
                case commands::LOGOUT_REQUEST:
                    break;
                        
                case commands::LOGOUT_RESPONSE:
                    packet << this->userid;
                    break;
                    
                case commands::MESSAGE_REQUEST:
                    packet << this->text;
                    break;
                    
                case commands::MESSAGE_RESPONSE:
                    packet << this->text << this->userid;
                    break;
                    
                case commands::USERLIST_UPDATE:
                    packet << this->add_user << this->userid
                           << this->username;
                    break;
                    
            }
            // Send packet
            auto status = socket.send(packet);
            if (status != sf::Socket::Done) {
                std::cerr << "Error while sending #" << this->command
                          << std::endl << std::flush;
                return false;
            }
            return true;
        }
        
        bool receive(sf::TcpSocket & socket) {
            // Receive packet
            sf::Packet packet;
            auto status = socket.receive(packet);
            if (status != sf::Socket::Done) {
                return false;
            }
            // Obtain data
            if (packet >> this->command) {
                bool success = false;
                switch (this->command) {
                
                    case commands::LOGIN_REQUEST:
                        success = (packet >> this->username);
                        break;
                        
                    case commands::LOGIN_RESPONSE:
                        success = (packet >> this->username >> this->userid
                                          >> this->success);
                        break;
                        
                    case commands::LOGOUT_REQUEST:
                        success = true; // nothing to read from the packet
                        break;
                        
                    case commands::LOGOUT_RESPONSE:
                        success = (packet >> this->userid);
                        break;
                    
                    case commands::MESSAGE_REQUEST:
                        success = (packet >> this->text);
                        break;
                    
                    case commands::MESSAGE_RESPONSE:
                        success = (packet >> this->text >> this->userid);
                        break;
                    
                    case commands::USERLIST_UPDATE:
                        success = (packet >> this->add_user >> this->userid
                                          >> this->username);
                        break;
                }
                if (!success) {
                    std::cerr << "Error while reading #" << this->command
                              << std::endl << std::flush;
                    return false;
                }
            } else {
                std::cerr << "Error while reading CommandID" << std::endl
                          << std::flush;
                return false;
            }
            return true;
        }

        std::string username;
        net::ClientID userid;
        bool success;
        std::string text;
        bool add_user;
};

#endif // COMMANDS_HPP_INCLUDED
