#include "Sockets.h"

namespace Sockets
{
#ifdef _WIN32
    bool init() { WSADATA wsaData; return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0; }
    bool cleanup() { return WSACleanup() == 0; }
#else
    bool init() { return true; }
    bool cleanup() { return true; }
#endif

    bool resolveHostname(const std::string& hostname, addrinfo*& addrOut, 
                        const std::string& port, bool listenSocket)
	{
        addrinfo hints{};
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM; // STREAM, DGRAM, etc.
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_family = AF_UNSPEC; // IPv4 (AF_INIT), IPv6 (AF_INET6), etc.
        if (listenSocket) { hints.ai_flags = AI_PASSIVE; }

        const char* h = hostname.empty() ? NULL : hostname.c_str();
		return getaddrinfo(h, port.c_str(), &hints, &addrOut) == 0;
	}

	bool connectSocket(addrinfo*& addr, SOCKET& socketOut)
	{
        for (addrinfo* p = addr; p; p = p->ai_next)
        {
            SOCKET s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (s == INVALID_SOCKET) { continue; }
            if (connect(s, p->ai_addr, (socklen_t)p->ai_addrlen) == SOCKET_ERROR) { close(s); continue; }
            if (s == INVALID_SOCKET) { continue; }

            socketOut = s;
            return true;
        }
        return false;
	}

    bool setupStream(const std::string& hostname, SOCKET& socketOut) 
    {
        addrinfo* hostAddr = nullptr;
        if (!resolveHostname(hostname, hostAddr)) { return false; }
        SOCKET s = 0;
        auto r = connectSocket(hostAddr, s);
        freeaddrinfo(hostAddr);
        socketOut = s;
        return r;
    }

    bool sendData(SOCKET& s, char* data, const size_t& dataSize)
    {
        return send(s, data, dataSize, 0) != SOCKET_ERROR;
    }

    bool listenSocket(SOCKET& s, const std::string& port)
    {
        addrinfo* p = nullptr;
        if (!resolveHostname(std::string(), p, port, true)) { return false; }
        SOCKET s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        auto r_b = (bind(s, p->ai_addr, (socklen_t)p->ai_addrlen) != SOCKET_ERROR);
        auto r_l = (listen(s, 1000) != SOCKET_ERROR);
        freeaddrinfo(p);
        if (s != INVALID_SOCKET) { return false; }
        if (!r_b && !r_l) { close(s); return false; }

    }

    RecStat::RecStat() : e{ RecStatE::NoOp } {};
    RecStat::RecStat(const int64_t& r)
    {
        if (r < 0) { e = RecStatE::Error; }
        else if (r == 0) { e = RecStatE::ConnectionClosed; }
        else {  size = r; e = RecStatE::Success; }
    }

    RecStat receiveData(SOCKET& s, char* outBuffer, size_t bufSize)
    { return RecStat(recv(s, outBuffer, bufSize, MSG_WAITALL)); }

    RecStat receiveData_CL(SOCKET& s, char& outBuffer, const size_t& bufSize,
                    sockaddr& srcAddrOut, size_t& srcAddrLenOut)
    { return RecStat(recvfrom(s, &outBuffer, bufSize, 0, &srcAddrOut, (socklen_t*)&srcAddrLenOut)); }

    bool shutdownConnection(const SOCKET& s, int flag) 
    { return shutdown(s, flag) != SOCKET_ERROR; }

    bool closeSocket(SOCKET s) 
    { return close(s) != SOCKET_ERROR; }

}