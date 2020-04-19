/***
Project 4
main
***/

#include "client.h"
#include "server.h"
#include "SocketSendRecvTools.h"
#include "SocketExampleShared.h"

int main(int argc, char* argv[])
{
	int openResult = 3, inputMode = 3; 
	SERVER_PORT = atoi(argv[3]);//define port
	
	if (0 == strcmp("server", argv[1])) //server mode
	{
		while (1)
		{
			openResult = openServer(argv[2]);
			if (-1 == openResult)
				exit(-1); //error
		}

	}
	else if (0 == strcmp("client", argv[1])) //client mode
	{
		inputMode = getUserMode(argv[4]);
		if (1 == inputMode) //file mode
			mainClient(inputMode, argv[2], argv[5]);
		else               //human mode
			mainClient(inputMode, argv[2], NULL);
	}
	return 0;
}