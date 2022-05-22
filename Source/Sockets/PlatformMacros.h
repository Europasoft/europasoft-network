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
	#ifndef SOCKET_ERROR
		#define SOCKET_ERROR -1
	#endif
#endif
#ifndef _Acquires_lock_()
	#define _Acquires_lock_()
#endif

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif