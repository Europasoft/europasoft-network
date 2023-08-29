#include "Sockets.h"
#include <cassert>
namespace Sockets
{
#ifdef _WIN32
    bool init() { WSADATA wsaData; return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0; }
    bool cleanup() { return WSACleanup() == 0; }
#else
    bool init() { return true; }
    bool cleanup() { return true; }
#endif

    bool resolveHostname(const std::string& hostname, bool numericHost, addrinfo*& addrOut, 
                        const std::string& port, bool numericPort, bool listenSocket)
	{
        addrinfo hints{};
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM; // STREAM, DGRAM, etc.
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_family = AF_INET; // IPv4 (AF_INET), IPv6 (AF_INET6), etc.
        hints.ai_flags =  (numericHost ? AI_NUMERICHOST : 0x0)
                        | (numericPort ? AI_NUMERICSERV : 0x0)
                        | (listenSocket ? AI_PASSIVE : 0x0);

        const char* h = listenSocket ? NULL : hostname.c_str();
        return (getaddrinfo(h, port.c_str(), &hints, &addrOut) == 0);
	}

	bool connectSocket(addrinfo*& addr, SOCKET& socketOut)
	{
        for (addrinfo* p = addr; p; p = p->ai_next)
        {
            if (p->ai_family != 2) { continue; }
            std::cout << "\nAttempting connection to " << getAddrAsString(p);
            SOCKET s = INVALID_SOCKET;
            s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (s == INVALID_SOCKET) { continue; }
            bool connected = (connect(s, p->ai_addr, (socklen_t)p->ai_addrlen) != SOCKET_ERROR);
            if (s == INVALID_SOCKET || !connected) { closeSocket(s); continue; }
            socketOut = s;
            return true;
        }
        return false;
	}

    bool setupStream(const std::string& hostname, const std::string& port, SOCKET& socketOut) 
    {
        addrinfo* hostAddr = nullptr;
        if (!resolveHostname(hostname, false, hostAddr, port, true)) { return false; }
        SOCKET s = INVALID_SOCKET;
        bool cr = connectSocket(hostAddr, s);
        //freeaddrinfo(hostAddr);
        socketOut = s;
        return cr;
    }

    bool sendData(SOCKET& s, const char* data, const size_t& dataSize)
    {
        return send(s, data, dataSize, 0) != SOCKET_ERROR;
    }

    bool listenSocket(SOCKET& s, const std::string& port)
    {
        addrinfo* p = nullptr;
        if (!resolveHostname(std::string(), true, p, port, true, true)) { return false; }
        SOCKET ls = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        setsockopt(ls, SOL_SOCKET, IPV6_V6ONLY, 0, sizeof(bool));
        auto r_b = (bind(ls, p->ai_addr, (socklen_t)p->ai_addrlen) != SOCKET_ERROR);
        auto r_l = (listen(ls, 1000) != SOCKET_ERROR);
        freeaddrinfo(p);
        if (!r_b || !r_l || ls == INVALID_SOCKET) { close(ls); return false; }
        s = ls;
        return true;
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

    long getReceiveSize(SOCKET s)
    {
#ifdef _WIN32
        unsigned long len = 0;
        ioctlsocket(s, FIONREAD, &len);
#else
        int len = 0;
        ioctl(s, FIONREAD, &len);
#endif
        return (long)len; // return at least 0
    }

    bool shutdownConnection(const SOCKET& s, int flag) 
    { return shutdown(s, flag) != SOCKET_ERROR; }

    void closeSocket(SOCKET s) 
    { if (s != INVALID_SOCKET) { close(s); } }

    std::string getAddrAsString(addrinfo* ai)
    {
        if (!ai) { return std::string(); }
        char* addrStr = new char[46];
        // inet_ntop() should convert the address from network-byte-order to host-byte-order
        inet_ntop(ai->ai_family, &((struct sockaddr_in*)ai->ai_addr)->sin_addr, addrStr, 46);
        std::string str = std::string(addrStr, strlen(addrStr));
        delete[] addrStr;
        return str;
    }
}