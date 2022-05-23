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
    while(!client.isConnected() || !server.isConnected()) {} // wait until connection acknowledged
        std::cout << "\nConnection established";

    for (;;)
    {
        std::string msg;
        std::cout << "\n\nCLIENT  >  ";
        std::getline(std::cin, msg);
        client.sendStream(msg.c_str(), msg.size());

        size_t resSize = 0;
        char resBuf[128];
        while (resSize <= 0)
        {
            resSize = server.getReceiveBuffer(resBuf, 128);
            if (resSize > 0) 
            {
                std::cout << "\nSERVER IN: " << std::string(resBuf, resSize);
            }
        }

        std::cout << "\n\nSERVER > ";
        std::getline(std::cin, msg);
        server.sendStream(msg.c_str(), msg.size());

        resSize = 0;
        while (resSize <= 0)
        {
            resSize = client.getReceiveBuffer(resBuf, 128);
            if (resSize > 0)
            {
                std::cout << "\nCLIENT IN: " << std::string(resBuf, resSize);
            }
        }


    }
}
