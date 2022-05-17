#pragma once
#include "Client.h"

/* SERVER EXECUTION PATH */
// 1. Initialize, resolve own address/port
// 2. Create listen socket, bind socket to own address/port
// 3. Listen for connection
// 4. On connection established, run inherited client code

class Server : public Client
{
	
};
