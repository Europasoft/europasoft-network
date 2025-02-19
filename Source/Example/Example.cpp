#include "Example.h"
#include <iostream>

//void ChatClient::connect(const std::string& port, const std::string& hostname)
//{
//#ifdef NET_SERVER_ONLY
//	// server connect
//	agent.listen(port);
//#else
//	// client connect
//	agent.connect(hostname, port);
//#endif
//}
//
//bool ChatClient::sendString(size_t i, const std::string& str)
//{
//    if (!connected(i)) { return false; }
//	agent.sendStream(i, str.c_str(), str.size());
//    return true;
//}
//
//std::string ChatClient::receiveString(size_t i, const uint32_t& retryMaxAttempts)
//{
//    if (!connected(i)) { return std::string(); }
//    uint32_t attemptCounter = 0;
//    while (attemptCounter < retryMaxAttempts)
//    {
//        attemptCounter++;
//        char buffer[256];
//        size_t size = agent.getReceiveBuffer(i, buffer, 256);
//        if (size > 0) { return std::string(buffer, size); }
//    }
//    return std::string();
//}

