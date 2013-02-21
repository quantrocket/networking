#ifndef SERVER_HPP
#define SERVER_HPP

#include "common.hpp"

// Server class
class Server {
    protected:
        EventPipe* in;
        EventPipe* out;
        bool running;
        std::string username;
        std::string history;
        SDL_Thread* thread;
        
        void login(LoginRequest* data);
        void message(MessageRequest* data);
        void logout(LogoutRequest* data);
        
    public:
        Server(EventPipe* in, EventPipe* out);
        ~Server();

        void handle();
};

int server_thread(void* param);

#endif
