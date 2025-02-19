#pragma once
#include "Sockets/Sockets.h"
#include <vector>
#include <memory>
#include <string>
#include <string_view>

class StreamThread;
class ListenThread;

class Connection
{
public:
    Connection(SOCKET connectedSocket); // server
    Connection(std::string_view hostname, std::string_view port); // client

    bool isConnected() const;
    bool isFailed() const;
    void Close();

    bool send(std::string_view data);
    void receive(std::string& data);

private:
    std::unique_ptr<StreamThread> thread;
};

/* CLIENT
* 1. Initialize, resolve host address
* 2. Establish connection to host
* 3. Send/receive through stream thread */

/* SERVER
* 1. Initialize, resolve own address
* 2. Create listen socket, bind socket to own address/port
* 3. Listen for connections
* 4. Commence send/receive on connected stream threads */

class Agent
{
public:
    enum class Mode { Server, Client };
    Agent(Mode mode);
    ~Agent();

    // establish TCP connection to a remote host, returns the index of the new connection
    size_t connect(std::string_view hostname, std::string_view port);

    // begin accepting connections, server mode only
    void listen(std::string_view port);
    // stop accepting connections, server mode only
    void stopListening();

    Connection& getConnection(size_t i);
    size_t numConnections();
    
    bool isServer() const { return listenThread.get(); }
    
protected:
    std::vector<Connection> connections;
    std::unique_ptr<ListenThread> listenThread;
    void updateConnections();
};