#pragma once
// dependencies
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#pragma comment(lib, "Ws2_32.lib")
#else
	// TODO: gnu/linux includes
#endif
// macros
#ifdef _WIN32
	#define close(s) closesocket(s)
#else
	#ifndef SOCKET
		#define SOCKET int
	#endif
	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET -1
	#endif
	#ifndef SOCKET_ERROR
		#define SOCKET_ERROR -1
	#endif
#endif
#ifndef _Acquires_lock_()
	#define _Acquires_lock_()
#endif

#include <string>
#include <mutex>
#define DEFAULT_PORT "27015" // default tcp/udp port

namespace Sockets
{
	using size_t = std::size_t;

	bool init();
	bool cleanup();

	// resolves address from hostname, remember to use freeaddrinfo() on the result, empty hostname returns localhost
	bool resolveHostname(const std::string& hostname, struct addrinfo*& addrOut, 
						const std::string& port = DEFAULT_PORT, bool listenSocket = false);

	// attempts to open a client socket and connect, remember to close the socket
	bool connectSocket(struct addrinfo*& addr, SOCKET& socketOut);

	// handles both hostname resolution and socket creation, establishes client-server TCP connection
	bool setupStream(const std::string& hostname, SOCKET& socketOut);
	
	// sends data over a socket
	bool sendData(SOCKET& s, char* data, const size_t& dataSize);

	bool listenSocket(SOCKET& s, const std::string& port = DEFAULT_PORT);

	enum class RecStatE { NoOp, Success, ConnectionClosed, Error };
	struct RecStat { RecStatE e; size_t size = 0; RecStat(const int64_t& r); RecStat(); };

	// receive (TCP), this is a blocking call
	RecStat receiveData(SOCKET& s, char* outBuffer, size_t bufSize);

	// connectionless receive (UDP)
	RecStat receiveData_CL(SOCKET& s, char& outBuffer, const size_t& bufSize,
						struct sockaddr& srcAddrOut, size_t& srcAddrLenOut);

	// shuts a connection down, flag can be one of: 0 (SD_RECEIVE), 1 (SD_SEND), 2 (SD_BOTH)
	bool shutdownConnection(const SOCKET& s, int flag);

	// completely closes a socket
	bool closeSocket(SOCKET s);

	// threadsafe socket handle, auto-closing
	class MutexSocket 
	{
		SOCKET s = 0;
		bool initialized = false;
		std::mutex m;
		
	public:
		using Lock = std::unique_lock<std::mutex>; // syntactic sugar (MutexSocket::Lock)
		MutexSocket() = default;
		MutexSocket(const SOCKET& s_) : s{ s_ } {};
		~MutexSocket() { closeSocket(s); }
		MutexSocket(const MutexSocket&) = delete; // no copying
		// ensures thread safety by locking the mutex (may be blocking)
		_Acquires_lock_(lock) const SOCKET& get(Lock& lock)
		{
			// tries to lock the mutex, which blocks (waits) here if mutex is locked by another thread (socket in use) 
			lock = std::move(std::unique_lock<std::mutex>(m)); // mutex will unlock when lock object is destroyed
			return s;
		}
		void set(const SOCKET& s_, bool forceReassign = false)
		{ 
			if (initialized && !forceReassign) { return; }
			std::unique_lock<std::mutex> lock{};
			get(lock);
			s = s_;
			initialized = true;
		}
	};
	
}



