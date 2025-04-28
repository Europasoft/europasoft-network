// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.
#ifdef _WIN32
	//#define close(s) closesocket(s)
	#pragma once
	#include <windows.h>
	#include <processthreadsapi.h>
	#define WIN_SET_THREAD_NAME(n) SetThreadDescription(GetCurrentThread(),n)
#else
	#define WIN_SET_THREAD_NAME(n)
#endif

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
#ifndef max
	#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef min
	#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif