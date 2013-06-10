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

#include <net/client.hpp>

namespace net {

    Client::Client()
        : CallbackManager<CommandID, json::Var &>() {
    }

    Client::~Client() {
        if (this->isOnline()) {
            this->disconnect();
        }
    }

    void Client::network_loop() {
        while (this->isOnline()) {
            // Send All JSON Objects
            while (true) {
                json::Var object = this->out.pop();
                if (object.isNull()) {
                    // Nothing Left
                    break;
                }
                // Serialize Object
                std::string dump = object.dump();
                // Send to Server
                try {
                    this->link.write(dump);
                } catch (const BrokenPipe& bp) {
                    std::cerr << "Connection to server was lost" << std::endl;
                    this->link.close();
                    return;
                }
            }
            // Receive All JSON Objects
            while (this->link.isReady()) {
                std::string dump;
                // Read Dumped Object
                try {
                    dump = this->link.read();
                } catch (const BrokenPipe& bp) {
                    std::cerr << "Connection to server was lost" << std::endl;
                    this->link.close();
                    return;
                }
                // Deserialize JSON Object
                json::Var object;
                object.load(dump);
                this->in.push(object);
            }
            utils::delay(25);
        }
    }

    void Client::handle_loop()  {
        while (this->isOnline()) {
            // wait for next object
            json::Var object = this->pop();
            if (object.isNull()) {
                // Null-Object
                utils::delay(15);
            } else {
                json::Var payload = object["payload"];
                CommandID command_id;
                if (!payload["command"].get(command_id)) {
                    continue;
                }
                this->trigger(command_id, payload);
            }
        }
    }

    void Client::connect(std::string const & ip, std::uint16_t const port) {
        if (this->isOnline()) {
            return;
        }
        // open connection
        this->link.open(ip, port);
        // receive client id
        std::string dump = this->link.read();
        json::Var welcome;
        welcome.load(dump);
        if (!welcome["id"].get(this->id)) {
            throw std::runtime_error("Did not get ClientID from server");
        }
        // start threads
        this->networker = std::thread(&Client::network_loop, this);
        // start handler
        this->handler = std::thread(&Client::handle_loop, this);
    }

    void Client::shutdown() {
        // wait until outgoing queue is empty
        // @note: data that is pushed while this queue is waiting might be lost
        while (this->isOnline() && !this->out.isEmpty()) {
            utils::delay(15);
        }
        this->disconnect();
    }

    void Client::disconnect() {
        // close connection
        this->link.close();
        this->networker.join();
        this->handler.join();
        // clear queues
        this->in.clear();
        this->out.clear();
    }

}
