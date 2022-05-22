#pragma once
// dependencies
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#pragma comment(lib, "Ws2_32.lib")
#else
	// TODO: gnu/linux includes
#endif
// contains macros to enable cross-compilation of socket code
#include "PlatformMacros.h" 

#include <string>
#include <mutex>

namespace Sockets
{
	using size_t = std::size_t;
	using Lock = std::unique_lock<std::recursive_mutex>; // syntactic sugar

	bool init();
	bool cleanup();

	// resolves address from hostname, remember to use freeaddrinfo() on the result, empty hostname returns localhost
	bool resolveHostname(const std::string& hostname, struct addrinfo*& addrOut, 
						const std::string& port, bool listenSocket = false);

	// attempts to open a client socket and connect, remember to close the socket
	bool connectSocket(struct addrinfo*& addr, SOCKET& socketOut);

	// handles both hostname resolution and socket creation, establishes client-server TCP connection
	bool setupStream(const std::string& hostname, const std::string& port, SOCKET& socketOut);
	
	// sends data over a socket
	bool sendData(SOCKET& s, const char* data, const size_t& dataSize);

	bool listenSocket(SOCKET& s, const std::string& port);

	enum class RecStatE { NoOp, Success, ConnectionClosed, Error };
	struct RecStat { RecStatE e; size_t size = 0; RecStat(const int64_t& r); RecStat(); };

	// receive (TCP), this is a blocking call
	RecStat receiveData(SOCKET& s, char* outBuffer, size_t bufSize);

	// connectionless receive (UDP)
	RecStat receiveData_CL(SOCKET& s, char& outBuffer, const size_t& bufSize,
						struct sockaddr& srcAddrOut, size_t& srcAddrLenOut);

	// gets the size of data available to be received, without blocking
	long getReceiveSize(SOCKET s);

	// shuts a connection down, flag can be one of: 0 (SD_RECEIVE), 1 (SD_SEND), 2 (SD_BOTH)
	bool shutdownConnection(const SOCKET& s, int flag);

	// completely closes a socket
	bool closeSocket(SOCKET s);

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
		// no copying, move only
		MutexSocket(const MutexSocket&) = delete; 
		MutexSocket(MutexSocket&& pr) noexcept
		{ 
			Lock l;
			s = pr.get(l); // move socket handle without closing socket
			initialized = true; 
			pr.s = INVALID_SOCKET;
			pr.initialized = false;
		}
		// ensures thread safety by locking the mutex (may be blocking)
		_Acquires_lock_(lock) const SOCKET& get(Lock& lock)
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



