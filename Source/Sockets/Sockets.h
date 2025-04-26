// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#pragma once
// dependencies
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#pragma comment(lib, "Ws2_32.lib")
#else
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <arpa/inet.h>
#endif
// contains macros to enable cross-compilation of socket code
#include "PlatformMacros.h" 

#include <string>
#include <mutex>

#include <iostream>

namespace Sockets
{
	using size_t = std::size_t;
	using Lock = std::unique_lock<std::recursive_mutex>; // syntactic sugar

	bool init();
	bool cleanup();
	size_t& socketsInitCounter();

	// resolves address from hostname, remember to use freeaddrinfo() on the result, empty hostname returns localhost
	bool resolveHostname(const std::string& hostname, bool numericHost, addrinfo*& addrOut,
						const std::string& port, bool numericPort, bool listenSocket = false);

	// attempts to open a client socket and connect, remember to close the socket
	bool connectSocket(struct addrinfo*& addr, SOCKET& socketOut);

	// handles both hostname resolution and socket creation, establishes client-to-server TCP connection
	bool setupStream(const std::string& hostname, const std::string& port, SOCKET& socketOut);
	
	// sends data over a socket
	bool sendData(SOCKET& s, const char* data, const size_t& dataSize);

	bool createListenSocket(SOCKET& s, const std::string& port, const std::string& hostname = std::string());

	// receive (TCP), this is a blocking call
	int32_t receiveData(SOCKET& s, char* outBuffer, size_t bufSize);

	// connectionless receive (UDP)
	int32_t receiveData_CL(SOCKET& s, char& outBuffer, const size_t& bufSize,
						struct sockaddr& srcAddrOut, size_t& srcAddrLenOut);

	// gets the size of data available to be received, without blocking
	long getReceiveSize(SOCKET s);

	// switches the socket to either blocking or non-blocking mode
	int32_t setBlocking(SOCKET s, bool block);

	// shuts a connection down, flag can be one of: 0 (SD_RECEIVE), 1 (SD_SEND), 2 (SD_BOTH)
	bool shutdownConnection(const SOCKET& s, int flag);

	// completely closes a socket
	void closeSocket(SOCKET s);

	std::string getAddrAsString(addrinfo* ai);

	void threadSleep(int milliseconds);

	// threadsafe socket handle, auto-closing
	class MutexSocket 
	{
		SOCKET s;
		bool initialized = false;
	public:
		std::recursive_mutex m;
		MutexSocket() : s{ INVALID_SOCKET } {};
		MutexSocket(const SOCKET& s_) : s{ s_ } {};
		~MutexSocket() { closeSocket(s); }
		MutexSocket(const MutexSocket&) = delete; // no copying, move only
		MutexSocket(MutexSocket&& pr) noexcept
		{ 
			Lock l;
			s = pr.get(l); // move socket handle without closing socket
			initialized = true; 
			pr.s = INVALID_SOCKET;
			pr.initialized = false;
		}
		// ensures thread safety by locking the mutex (may block)
#ifdef _Acquires_lock_()
		_Acquires_lock_(lock) 
#endif
		const SOCKET& get(Lock& lock)
		{
			// tries to lock the mutex, which blocks (waits) here if mutex is locked by another thread (socket in use) 
			lock = std::move(std::unique_lock<std::recursive_mutex>(m)); // mutex will unlock when lock object is destroyed
			return s;
		}
		void set(const SOCKET& s_, bool forceReassign = false)
		{ 
			if (initialized && !forceReassign) { return; }
			Lock lock{};
			get(lock);
			s = s_;
			initialized = (s != INVALID_SOCKET);
		}
		bool isInitialized() const { return initialized; }
	};

	
}



