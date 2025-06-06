// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#include "Sockets.h"

#include <cassert>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>

namespace Sockets
{
#ifdef _WIN32
    bool init()
	{
		socketsInitCounter()++;
		if (socketsInitCounter() > 1)
			return true;
		WSADATA wsaData;
		return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
	}
    bool cleanup() 
	{
		socketsInitCounter() -= (socketsInitCounter() > 0) ? 1 : 0;
		return (socketsInitCounter() < 1) ? WSACleanup() == 0 : true;
	}
#else
    bool init() { return true; }
    bool cleanup() { return true; }
#endif
	size_t& socketsInitCounter()
	{
		static size_t count = 0;
		return count;
	}

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
        freeaddrinfo(hostAddr); // TODO: why was this commented out before?
        socketOut = s;
        return cr;
    }

    size_t sendData(SOCKET& s, const char* data, const size_t& dataSize)
    {
		const size_t bytesSent = send(s, data, dataSize, 0);
		return (bytesSent != SOCKET_ERROR) ? bytesSent : 0;
    }

    bool createListenSocket(SOCKET& s, const std::string& port, const std::string& hostname)
    {
        addrinfo* p = nullptr;
        if (!resolveHostname(hostname, true, p, port, true, true)) { return false; }
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        setsockopt(s, SOL_SOCKET, IPV6_V6ONLY, 0, sizeof(bool));
        auto bound = (bind(s, p->ai_addr, (socklen_t)p->ai_addrlen) != SOCKET_ERROR);
        auto listening = (listen(s, 100) != SOCKET_ERROR);
        freeaddrinfo(p);
        if (!bound || !listening || s == INVALID_SOCKET) { closeSocket(s); return false; }
        return true;
    }

    int32_t receiveData(SOCKET& s, char* outBuffer, size_t bufSize)
    { 
        return recv(s, outBuffer, bufSize, MSG_WAITALL); 
    }

    int32_t receiveData_CL(SOCKET& s, char& outBuffer, const size_t& bufSize,
                    sockaddr& srcAddrOut, size_t& srcAddrLenOut)
    { 
        return recvfrom(s, &outBuffer, bufSize, 0, &srcAddrOut, (socklen_t*)&srcAddrLenOut); 
    }

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

    int32_t setBlocking(SOCKET s, bool block) 
    {
        int nonBlocking = !block;
        int r = SOCKET_ERROR;
#ifdef _WIN32
        r = ioctlsocket(s, FIONBIO, (unsigned long*)&nonBlocking);
#else
        r = ioctl(s, FIONBIO, &nonBlocking);
#endif
        return r;
    }

	bool setReceiveTimeout(SOCKET socketFd, int timeoutMs) 
	{
#ifdef _WIN32
		DWORD timeout = static_cast<DWORD>(timeoutMs);
		if (setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO,
			reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == SOCKET_ERROR)
			return false;
#else
		struct timeval timeout{};
		timeout.tv_sec = timeoutMs / 1000;
		timeout.tv_usec = (timeoutMs % 1000) * 1000;
		if (setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO,
			&timeout, sizeof(timeout)) != 0)
			return false;
#endif
		return true;
	}

    bool shutdownConnection(const SOCKET& s, int flag) { return shutdown(s, flag) != SOCKET_ERROR; }

	void closeSocket(SOCKET s)
	{ 
		if (s != INVALID_SOCKET)
#ifdef _WIN32
		closesocket(s);
#else
		close(s);
#endif
	}

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
    void threadSleep(int milliseconds)
    {
		assert(not (milliseconds < 0 || milliseconds > 1000000));
		std::this_thread::sleep_for(std::chrono::duration<long long, std::milli>(milliseconds));
    }
}