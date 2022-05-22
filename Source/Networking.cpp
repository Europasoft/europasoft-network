// Networking.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "NetAgent/Server.h"
#include <iostream>

int main()
{
    std::cout << "\nInitializing...";
    Server server{};
    Client client{};  client.noRecv();
    std::cout << "OK\nSERVER...";
    server.listenStart("5001");
    std::cout << "OK";
    std::cout << "\nCLIENT...";
    client.connectStream("localhost", "5001");
    std::cout << "OK";
    std::cout << "\nConnecting...";
    bool connected = false;
    while (!server.checkConnection()) {}
    std::cout << "OK\nSERVER Connection established";

    char* resBuf = new char[128];
    size_t resSize = 0;

    for (;;) 
    {   resSize = server.getReceiveBuffer(resBuf, 128);
        if (resSize > 0) 
        {
            std::cout << "\nSERVER IN: " << std::string(resBuf, resSize);
            resSize = 0;
        }

        std::string msg;
        std::cout << "\n\nCLIENT  >  ";
        std::getline(std::cin, msg);
        client.sendStream(msg.c_str(), msg.size());
    }
}
