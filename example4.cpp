/*
Copyright © 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers an event-based networking framework for games and other software.

The source code is released under CC BY-NC 3.0
http://creativecommons.org/licenses/by-nc/3.0/
*/

// networking 4 gewinnt
#include <iostream>
#include <string>

#include "src/connection.hpp"
#include "src/eventsystem.hpp"

// --- Game Logic ---

const char RED    = 'R';
const char YELLOW = 'Y';
const char EMPTY  = ' ';

class ConnectFour {
    protected:
        char field[6][7]; // x, y
    public:
        ConnectFour();
        virtual ~ConnectFour();
        void print();
        bool turn(char color, unsigned short x);
};

ConnectFour::ConnectFour() {
    for (unsigned short y = 0; y < 7; y++) {
        for (unsigned short x = 0; x < 6; x++) {
            this->field[x][y] = EMPTY;
        }
    }
}

ConnectFour::~ConnectFour() {
}

void ConnectFour::print() {
    std::cout << "  0   1   2   3   4   5\n";
    for (unsigned short y = 0; y < 7; y++) {
        std::cout << ". - . - . - . - . - . - .\n" << "| ";
        for (unsigned short x = 0; x < 6; x++) {
            std::cout << this->field[x][y] << " | ";
        }
        std::cout << "\n";
    }
    std::cout << ". - . - . - . - . - . - .\n";
}

bool ConnectFour::turn(char color, unsigned short x) {
    if (color != RED && color != YELLOW) {
        std::cout << "Invalid player\n";
    }
    if (x < 0 || x >= 6) {
        std::cout << "Invalid column\n";
    }
    for (short y = 6; y >= 0; y--) {
        if (this->field[x][y] == EMPTY) {
            this->field[x][y] = color;
            return true;
        }
    }
    std::cout << "No space in column\n";
    return false;
}

// --- Communication Stuff ---

const EventID TURN = 1;
const EventID QUIT = 2;

class Turn: public Event {
    public:
        char player;
        int column;
        Turn(char player, int column)
            : Event(TURN)
            , player(player)
            , column(column) {
        }
        Turn(Turn* other)
            : Event(TURN)
            , player(other->player)
            , column(other->column) {
        }
};

class Quit: public Event {
    public:
        char winner;
        Quit(char winner)
            : Event(QUIT)
            , winner(winner) {
        }
        Quit(Quit* other)
            : Event(QUIT)
            , winner(other->winner) {
        }
};

Event* Event::assemble(void* buffer) {
    Event* event = reinterpret_cast<Event*>(buffer);
    EventID id = event->event_id;
    switch (id) {
        case TURN:
            event = new Turn((Turn*)buffer);
            break;
        case QUIT:
            event = new Quit((Quit*)buffer);
            break;
    };
    return event;
}

// --- Game Server (always player RED) ---

void server_func(unsigned short port) {
    TcpListener listener;
    TcpLink* link = NULL;
    NetworkingQueue* queue = NULL;
    // wait for opponent
    listener.open(port);
    while (link == NULL) {
        link = listener.accept();
    }
    queue = new NetworkingQueue(link);
    // start game
    ConnectFour* game = new ConnectFour();
    char player = RED;
    std::string input;
    int col;
    bool success;
    while (true) {
        // wait for server's turn
        game->print();
        std::cout << "Your turn! Which column?: ";
        std::getline(std::cin, input);
        col = std::stoi(input);
        success = game->turn(RED, col);
        if (!success) {
            // move not possible
            continue;
        }
        queue->push(new Turn(RED, col));
        // wait for client's turn
        game->print();
        std::cout << "Waiting for client ...\n";
        Event* event = NULL;
        while (event == NULL) {
            event = queue->pop();
        }
        if (event->event_id == TURN) {
            Turn* data = (Turn*)event;
            success = game->turn(YELLOW, data->column);
            delete event;
            if (!success) {
                // client thought this would work, so server wins
                // (stupid solution but suitable for test purpose^^)
                queue->push(new Quit(RED));
                break;
            }
        } else if (event->event_id == QUIT) {
            Quit* data = (Quit*)event;
            std::cout << "Game over. The winner is " << data->winner << "\n";
            delete event;
            break;
        }
    }
    // clean up
    link->close();
    delete queue;
    delete game;
}

// --- Game Client (always player YELLOW) ---

void client_func(const std::string& hostname, unsigned short port) {
    NetworkingQueue* queue;
    TcpLink link;
    // connect to server
    link.open(hostname, port);
    queue = new NetworkingQueue(&link);
    // start game
    ConnectFour* game = new ConnectFour();
    char player = YELLOW;
    std::string input;
    int col;
    bool success;
    while (true) {
        // wait for server's turn
        game->print();
        std::cout << "Waiting for server ...\n";
        Event* event = NULL;
        while (event == NULL) {
            event = queue->pop();
        }
        if (event->event_id == TURN) {
            Turn* data = (Turn*)event;
            success = game->turn(RED, data->column);
            delete event;
            if (!success) {
                // server thought this would work, so client wins
                // (stupid solution but suitable for test purpose^^)
                queue->push(new Quit(YELLOW));
                break;
            }
        } else if (event->event_id == QUIT) {
            Quit* data = (Quit*)event;
            std::cout << "Game over. The winner is " << data->winner << "\n";
            delete event;
            break;
        }
        while (true) {
            // wait for client's turn
            game->print();
            std::cout << "Your turn! Which column?: ";
            std::getline(std::cin, input);
            col = std::stoi(input);
            success = game->turn(YELLOW, col);
            if (!success) {
                // move not possible
                continue;
            }
            queue->push(new Turn(YELLOW, col));
            break;
        }
    }
    // clean up
    link.close();
    delete queue;
    delete game;
    
}

int main(int argc, char **argv) {
    std::string input;
    if (argc == 2) {
        unsigned short port = (unsigned short)(atoi(argv[1]));
        server_func(port);
    } else if (argc == 3) {
        unsigned short port = (unsigned short)(atoi(argv[2]));
        client_func(argv[1], port);
    } else {
        std::cout << "Usage:" << std::endl
                  << "\tdemo hostname port\t(start client)" << std::endl
                  << "\tdemo port\t\t(start server)" << std::endl;
    }
}


