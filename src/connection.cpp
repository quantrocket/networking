#include "connection.hpp"

TcpConnection::TcpConnection()
    : socket(NULL)
    , serve(false) {
}

TcpConnection::TcpConnection(TCPsocket socket)
    : socket(socket)
    , serve(false) {
}

TcpConnection::~TcpConnection() {
    if (this->socket != NULL) {
        this->disconnect();
    }
}

void TcpConnection::listen(unsigned short port) {
    if (this->socket != NULL) {
        throw NetworkError("TCP Socket already connected");
    }
    IPaddress ip;
    if (SDLNet_ResolveHost(&ip, NULL, port) == -1) {
        throw NetworkError("SDLNet_ResolveHost: " + std::string(SDLNet_GetError()));
    }
    this->socket = SDLNet_TCP_Open(&ip);
    if (this->socket == NULL) {
        throw NetworkError("SDLNet_TCP_Open: " + std::string(SDLNet_GetError()));
    }
    this->serve = true;
}

void TcpConnection::connect(const std::string& host, unsigned short port) {
    if (this->socket != NULL) {
        throw NetworkError("TCP Socket already connected");
    }
    IPaddress ip;
    if (SDLNet_ResolveHost(&ip, host.c_str(), port) == -1) {
        throw NetworkError("SDLNet_ResolveHost: " + std::string(SDLNet_GetError()));
    }
    this->socket = SDLNet_TCP_Open(&ip);
    if (this->socket == NULL) {
        throw NetworkError("SDLNet_TCP_Open: " + std::string(SDLNet_GetError()));
    }
    this->serve = false;
}

void TcpConnection::disconnect() {
    if (this->socket == NULL) {
        throw NetworkError("TCP Socket already closed");
    }
    SDLNet_TCP_Close(this->socket);
}

TcpConnection* TcpConnection::accept() {
    if (this->socket == NULL) {
        throw NetworkError("TCP Socket not connected");
    }
    if (!this->serve) {
        throw NetworkError("TCP Socket is not listening");
    }
    TCPsocket tmp = SDLNet_TCP_Accept(this->socket);
    if (tmp == NULL) {
        return NULL;
    }
    return new TcpConnection(tmp);
}

void TcpConnection::send(void* data, int len) {
    if (this->socket == NULL) {
        throw NetworkError("TCP Socket not connected!");
    }
    if (this->serve) {
        throw NetworkError("TCP Socket is listening");
    }
    // write byte amount
    SDLNet_TCP_Send(this->socket, &len, sizeof(int));
    // write actual data
    if (SDLNet_TCP_Send(this->socket, data, len) < len) {
        throw NetworkError("SDLNet_TCP_Send: " + std::string(SDLNet_GetError()));
    }
}

void* TcpConnection::receive() {
    if (this->socket == NULL) {
        throw NetworkError("TCP Socket not connected");
    }
    if (this->serve) {
        throw NetworkError("TCP Socket is listening");
    }
    // read byte amount
    int len;
    SDLNet_TCP_Recv(this->socket, &len, sizeof(int));
    // allocate memory
    void* buffer = malloc(len);
    // read actual data
    if (SDLNet_TCP_Recv(this->socket, buffer, len) < len) {
        throw NetworkError("SDLNet_TCP_Recv: " + std::string(SDLNet_GetError()));
    }
    return buffer;
}


