// Networking.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "NetAgent/Server.h"
#include <iostream>

int main()
{
    std::cout << "\nInitializing...";
    Server server{};
    Client client{};
    std::cout << "OK\nSERVER...";
    server.listenStart("5001");
    std::cout << "OK";
    std::cout << "\nCLIENT...";
    client.connectStream("localhost", "5001");
    std::cout << "OK";
    std::cout << "\nConnecting...";
    bool connected = false;
    while (!server.checkConnection()) {}
    std::cout << "OK\nConnection OK";
    
    std::cout << "\n\n[CLIENT] Enter a message: ";
    std::string msg;
    std::cin >> msg;
    client.sendStream(msg.c_str());

    std::string msg_in{};
    while (!server.getReceiveBuffer_String(msg_in)) {}
    std::cout << "\n[SERVER] data from peer: " << msg_in;

    for (;;) {}
}
