// Copyright 2025 Simon Liimatainen, Europa Software. All rights reserved.

// The example functions are defined in these files
#include "Examples/TcpChatExample.h"
#include "Examples/HttpServerExample.h"

#include <iostream>
#include <string>
#include <vector>

int main()
{
	// Run an example here, inspect the functions to see how they were implemented
	
	//return tcpChatExample();

	return httpServerExample("C:/YourWebrootPathHere");
}