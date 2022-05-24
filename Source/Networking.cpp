#include "NetAgent/Server.h"
#include "Example/Example.h"
#include <iostream>

int main()
{
    // TEST TEMPORARY (support for direct connect by IP - work in progress) -------
    Sockets::init();
    addrinfo* ai = nullptr;
    if (!Sockets::resolveHostname("localhost", ai, "5001")) { std::cout << "hostname resolve failed"; }
    auto* sa = ai->ai_addr;
    std::cout << std::string(sa->sa_data, ai->ai_addrlen);

    std::cout << "\ntest done";
    // END TEST -------------------------------------------------------------------
    std::cin.ignore();
    ChatClient client{};

    std::string h{};
#ifndef NET_SERVER_ONLY
    std::cout << "\nEnter hostname for remote machine: ";
    std::getline(std::cin, h);
    if (h.empty() || (h.find_first_not_of(" ") == h.npos)) { h = "localhost"; }
#endif
    std::string p{};
    std::cout << "Enter port number: ";
    std::getline(std::cin, p);
    if (p.empty() || (p.find_first_not_of(" ") == p.npos)) { p = "5001"; }

#ifdef NET_SERVER_ONLY
    std::cout << "\nWaiting for remote...";
#else
    std::cout << "\nConnecting...";
#endif
    client.connect(p, h);

    bool confail = false;
    while (!client.connected())
    {
        confail = client.failed();
        if (confail) { std::cout << "\n\nFailed to connect"; std::cin.ignore(); exit(1); }
    }
    std::cout << " Connection established";

    std::cout << "\n(leave message blank and press enter to receive)\n";
    for (;;)
    {
        auto instr = client.receiveString();
        if (!instr.empty()) { std::cout << "\nMessage from remote machine: " << instr << "\n"; }

        std::string msg{};
        std::cout << "\nEnter new message: ";
        std::getline(std::cin, msg);
        if (!msg.empty()) { client.sendString(msg); }
    }
}
