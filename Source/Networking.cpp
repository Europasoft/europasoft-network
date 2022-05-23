// Networking.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "NetAgent/Server.h"
#include <iostream>

int main()
{
    Server server{};
    Client client{};
    std::cout << "\nConnecting...";
    server.listenStart("5001");
    client.connectStream("localhost", "5001");

    while (!server.checkConnection()) {}
    std::cout << "\nConnected";

    for (;;)
    {
        std::string msg;
        std::cout << "\n\n > ";
        std::getline(std::cin, msg);
        client.sendStream(msg);

        std::string sin = server.getReceiveBuffer();
        if (!sin.empty()) std::cout << "\nRECV: " << sin;
    }
}
