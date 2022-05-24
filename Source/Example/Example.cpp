#include "Example.h"

void ChatClient::connect(const std::string& port, const std::string& hostname)
{
#ifdef NET_SERVER_ONLY
	// server connect
	agent.listenStart(port);
#else
	// client connect
	agent.connectStream(hostname, port);
#endif
}

bool ChatClient::sendString(const std::string& str)
{
    if (!connected()) { return false; }
	agent.sendStream(str.c_str(), str.size());
}

std::string ChatClient::receiveString(const uint32_t& retryMaxAttempts)
{
    if (!connected()) { return std::string(); }
    uint32_t attemptCounter = 0;
    while (attemptCounter < retryMaxAttempts)
    {
        attemptCounter++;
        char buffer[256];
        size_t size = agent.getReceiveBuffer(buffer, 256);
        if (size > 0) { return std::string(buffer, size); }
    }
    return std::string();
}