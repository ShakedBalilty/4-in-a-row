/***
Project 4
c file of server
***/
#include "server.h"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~global variables declaration~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int boardArray[BOARD_HEIGHT][BOARD_WIDTH];
int turnFlag; //who's turn is it now
int isGameStartedFlag=-1;
FILE *fPtrLog;
HANDLE logFileMutex;
SOCKET SockClient1 = INVALID_SOCKET; 
SOCKET SockClient2 = INVALID_SOCKET;
char clientsUsernames[NUM_OF_USERS][LENGTH_OF_USERNAME]; 
HANDLE  hClient1Thread, hClient2Thread, hClient3Thread; //the handles for the three communication thread
int SERVER_PORT;
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/************************************************************************************************
Description:
			this function is a simplified API for creating threads.
Parameters:
			input- LPTHREAD_START_ROUTINE *StartAddress: a pointer to the function to be executed by the thread.
				   This pointer represents the starting address of the thread.
				   VOID *Parameter: argument to thread function
			output- DWORD *ThreadId: a pointer to a variable that receives the thread identifier.
					HANDLE *h_thread: if the function succeeds, this is a handle to the new thread.
Returns:
			int- 0 if CreateThread() succeeded, -1 otherwise
************************************************************************************************/
int createThreadSimple(LPTHREAD_START_ROUTINE *StartAddress, VOID *Parameter, DWORD *ThreadId, HANDLE *h_thread)
{
	*h_thread = NULL;
	int status = 0;
	if (StartAddress == NULL || ThreadId == NULL)
	{
		status = -1;
		printf("Creating thread failed- received null pointer\n");
	}
	else
	{
		*h_thread = CreateThread(
			NULL, /* default security attributes */
			0, /* default stack size */
			StartAddress, /* thread function */
			Parameter, /* argument to thread function */
			0, /* default creation flags */
			ThreadId); /* returns the thread identifier */
	}
	if (*h_thread == NULL) // Create thread failed
	{
		status = -1;
		printf("Create thread failed");
	}
	return status;
}

/************************************************************************************************
Description:
			this function check if the play is legal and fill checkStatus if it doesn't
Parameters:
			input- int playerNum: the client who want to make the move
				   int columnPlay: the column to update in the playboard array
			output- char **checkStatus: pointer to string to fill with the explanation for the illegal move
Returns:
			int. -1 if illegal, 0 if legal
************************************************************************************************/
int checkLegalPlay(int playerNum, int columnPlay, char **checkStatus)
{
	columnPlay--; // adjust the column number to array from 0 to 6

	if (0 == isGameStartedFlag) //game has not started yet
	{
		strcpy(checkStatus, "Game; ;has; ;not; ;started");
		return -1;
	}
	
	if (playerNum != turnFlag) //error, not your turn
	{
		strcpy(checkStatus, "Not; ;your; ;turn");
		return -1;
	}
	if ((0 > columnPlay) || (columnPlay >= BOARD_WIDTH))// error, coordiante is not good
	{
		strcpy(checkStatus, "Illegal; ;move");
		return -1;
	}
	if (0 != boardArray[BOARD_HEIGHT - 1][columnPlay]) // error, column is full
	{
		strcpy(checkStatus, "Illegal; ;move");
		return -1;
	}
	return 0; //legal play
}

/************************************************************************************************
Description:
			the function creates the two connection threads for each of the clients and the thread to
			decline third connection
Parameters:
			input- none
			output- none
Returns:
			0 if succeeded, -1 otherwise
*************************************************************************************************/
int createThreads()
{
	DWORD   dwClient1ThreadId, dwClient2ThreadId, dwClient3ThreadId;
	int CreateError = 0; int i, j;
	int Ind;
	int Loop;
	unsigned long Address;
	SOCKADDR_IN service;
	int bindRes; int StartupRes;
	int ListenRes;
	WSADATA wsaData;
	TransferResult_t RecvRes;
	SOCKET MainSocket = INVALID_SOCKET; SOCKET AcceptSocket;
	HANDLE threadsArray[2];
	
	turnFlag = 0; //the game didn't start yet
	isGameStartedFlag = 0;//the game didn't start yet

	// Initialize Winsock.
	StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (StartupRes != NO_ERROR)
	{
		if (-1 == writeToLogFileAndPrint("can't initialize winsock, exiting", 1))// Tell the user that we could not find a usable WinSock DLL.    
			return -1;
		exit(-1);
	}
	// The WinSock DLL is acceptable. Proceed.

	// Create a socket.    
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (MainSocket == INVALID_SOCKET)
	{
		if (-1 == writeToLogFileAndPrint("can't create a socket, exiting", 1))
			return -1;
		exit(-1);
	}

	// Bind the socket.
   // Create a sockaddr_in object and set its values.
   // Declare variables

	Address = inet_addr(SERVER_ADDRESS_STR);
	if (Address == INADDR_NONE)
	{
		if (-1 == writeToLogFileAndPrint("can't convert the ip address, exiting", 1))
			return -1;
		exit(-1);
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;
	service.sin_port = htons(SERVER_PORT); //The htons function converts a u_short from host to TCP/IP network byte order 
								   //( which is big-endian ).

	// Call the bind function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR)
	{
		if (-1 == writeToLogFileAndPrint("bind failed, exiting", 1))
			return -1;
		exit(-1);
	}

	// Listen on the Socket.
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		if (-1 == writeToLogFileAndPrint("Failed listening on socket, exiting", 1))
			return -1;
		exit(-1);
	}

	//accept connection from client 1
	AcceptSocket = accept(MainSocket, NULL, NULL);
	if (AcceptSocket == INVALID_SOCKET)
	{
		if (-1 == writeToLogFileAndPrint("Accepting connection with client failed, exiting", 1))
			return -1;
		exit(-1);
	}
	else //accept connection from client to server's socket 1
	{
		SockClient1 = AcceptSocket;// shallow copy: don't close AcceptSocket, instead close SockClient1 when the time comes.
		CreateError = registerUsername(1, &SockClient1); //register the client by its ursername
		if (-1 == CreateError)
			return -1;
		CreateError = createThreadSimple(startPlayConnection, 1, &dwClient1ThreadId, &hClient1Thread);// Create the connection thread of client 1
		if (0 != CreateError)
		{ 
			if (-1 == writeToLogFileAndPrint("An error occurred while opening thread!", 1))
				exit(-1);
			return -1;	// Check the return value for success. If CreateThread fails, terminate execution. 
		}
	}
	//accept connection from client 2
	while (1)
	{
		AcceptSocket = accept(MainSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET)
		{
			if (-1 == writeToLogFileAndPrint("Accepting connection with client failed, exiting", 1))
				return -1;
			exit(-1);
		}
		else //accept connection from client to server's socket 2
		{
			SockClient2 = AcceptSocket;// shallow copy: don't close AcceptSocket, instead close SockClient2 when the time comes.
			CreateError = registerUsername(2, &SockClient2); //register the client by its username
			if (-1 == CreateError)
				return -1;
			else if (2 == CreateError)
				continue; //new user declined, wait for another user to register
			else
			{
				CreateError = createThreadSimple(startPlayConnection, 2, &dwClient2ThreadId, &hClient2Thread);// Create the connection thread of client 1
				if (0 != CreateError)
				{
					if (-1 == writeToLogFileAndPrint("An error occurred while opening thread!", 1))
						exit(-1);
					return -1;	// Check the return value for success. If CreateThread fails, terminate execution. 
				}
				break; //the user registered successfully
			}
		}
	}

	//open the thread that declines connections from third users
	CreateError = createThreadSimple(declineOtherConnections, MainSocket, &dwClient3ThreadId, &hClient3Thread);// Create the decline connection thread 
	if (0 != CreateError)
	{
		if (-1 == writeToLogFileAndPrint("An error occurred while opening thread!", 1))
			exit(-1);
		return -1;	// Check the return value for success. If CreateThread fails, terminate execution. 
	}

	turnFlag = 1; //game starts, client 1 turn
	isGameStartedFlag = 1;//update that both clients connected and can start play
	threadsArray[0] = hClient1Thread; 
	threadsArray[1] = hClient2Thread;

	WaitForMultipleObjects(2, threadsArray, FALSE, INFINITE);// wait for one connection thread to end

	//free all succsessfully opened thread handles
	TerminateThread(hClient1Thread, 0);
	CloseHandle(hClient1Thread);
	TerminateThread(hClient2Thread, 0);
	CloseHandle(hClient2Thread);
	TerminateThread(hClient3Thread, 0);
	CloseHandle(hClient3Thread);
	shutdown(MainSocket, SD_BOTH); //shut down the main socket that receive connections
	shutdownSockets();
	if (closesocket(SockClient1) == SOCKET_ERROR )
	{
		if (-1 == writeToLogFileAndPrint("Failed to close Socket! Ending program", 1))
			exit(-1);
	}
	if (closesocket(SockClient2) == SOCKET_ERROR )
	{
		if (-1 == writeToLogFileAndPrint("Failed to close Socket! Ending program", 1))
			exit(-1);
	}
	if (closesocket(MainSocket) == SOCKET_ERROR) 
	{
		if (-1 == writeToLogFileAndPrint("Failed to close Socket! Ending program", 1))
			exit(-1);
	}
	if ( WSACleanup() == SOCKET_ERROR) {
		if (-1 == writeToLogFileAndPrint("Failed to close Winsocket! Ending program", 1))
			exit(-1);
	}
	fclose(fPtrLog);// close log file
	return 0;
}

/************************************************************************************************
Description:
			this function opens the server program from the main
Parameters:
			input- char *pathOfLogFile: the path to the log file of the server to write to
			output- none
Returns:
			0 if succeeded, -1 otherwise
************************************************************************************************/
int openServer(char *pathOfLogFile)
{
	fPtrLog = fopen(pathOfLogFile, "w"); //open log file to write, erase old data on file
	if (fPtrLog == NULL) {
		printf("An error occurred while opening log file to write!");
		exit(-1);    //opening file failed
	}
	//create log file mutex
	logFileMutex = CreateMutex( 
		NULL,   // default security attributes 
		FALSE,	// don't lock mutex immediately 
		NULL);  // un-named 
	if (logFileMutex == NULL)
	{
		if(-1 == writeToLogFileAndPrint("creation of Mutex Array failed!", 1));
			exit(-1); //return error
		exit(-1);
	}
	initializeBoardArray(boardArray); //initialize board array for a new game
	if(-1 == createThreads()) // accept connections from clients and open the communication threads for client1 and client2
		return -1;//error
	return 0;
}

/************************************************************************************************
Description:
			this function register a client with a legal username that is sent to the server. 
			sends message to client accordingly
Parameters:
			input- int numOfClient: the client number that want to register
				   SOCKET *SockClient: the socket in the server that connects to this client
			output- none
Returns:
			0 if the function ended witout errors, 2 if the username was declined, -1 if ended with errors
************************************************************************************************/
int registerUsername(int numOfClient, SOCKET *SockClientPtr)
{
	char *AcceptedUsername = NULL;
	char *strPtrHelper = NULL;
	TransferResult_t RecvRes;
	char sendMessageString[LENGTH_OF_MESSAGE];

	if (1 == numOfClient)
		strcpy(sendMessageString, "NEW_USER_ACCEPTED:1"); // acception message for client 1
	if (2 == numOfClient)
		strcpy(sendMessageString, "NEW_USER_ACCEPTED:2"); // acception message for client 2
	if (-1 == receiveMessageProcess(&AcceptedUsername, SockClientPtr)) //receive message
		return -1;
	//received message
	
	strPtrHelper = strstr(AcceptedUsername, ":"); 
	strPtrHelper++;
	strcpy(AcceptedUsername, strPtrHelper); //copy only the username, including '\n' at the end
	strPtrHelper = strstr(AcceptedUsername, "\n");
	*strPtrHelper = '\0';

	if ((2 == numOfClient) && (0 == strcmp(AcceptedUsername, clientsUsernames[0]))) //new username equal to first username registered, decline
	{
		if(-1 == sendMessageProcess("NEW_USER_DECLINED", SockClientPtr)) //send to user that the request declined
			return -1;
		return 2;
	}
	else //accept username: username entered is ok, register the client  
	{
		strcpy(clientsUsernames[numOfClient - 1], AcceptedUsername); //copy username to the usernames array
		if (-1 == sendMessageProcess(sendMessageString, SockClientPtr)) //send to user that the request accepted
			return -1;
	}
	free(AcceptedUsername);
	return 0;
}

/************************************************************************************************
Description:
			this function send a message from the server to one of the clients, including error 
			checking, printing and log writing
Parameters:
			input- char *stringToSend: the string to connect
				   SOCKET *sendSocket: the socket in the server that connects to this client
			output- none
Returns:
			0 if the function ended witout errors, -1 otherwise
************************************************************************************************/
int sendMessageProcess(char *stringToSend, SOCKET *sendSocket)
{
	TransferResult_t sendRes;
	DWORD waitCode; BOOL releaseRes;

	sendRes = SendString(stringToSend, *sendSocket);
	if (sendRes == TRNS_FAILED) //error while sending, closing
	{
		waitCode = WaitForSingleObject(logFileMutex, INFINITE); //raise log file mutex
		if (WAIT_OBJECT_0 != waitCode)
		{
			if (-1 == writeToLogFileAndPrint("Error when waiting for mutex", 1))
				return -1;
			exit(-1);
		}
		if (-1 == writeToLogFileAndPrint("Service socket error while writing, exiting", 1))
			return -1;
		releaseRes = ReleaseMutex(logFileMutex); //release log file mutex
		if (releaseRes == FALSE)
		{
			if (-1 == writeToLogFileAndPrint("Error when releasing mutex", 1))
				return -1;
			exit(-1);
		}
		exit(-1);
	}
	return 0;
}

/************************************************************************************************
Description:
			this function receive a message from the client, including error checking, printing and log writing
Parameters:
			input- SOCKET *receiveSocket: the socket in the server that connects to this client
			output- char **stringToReceive: pointer to a string to copy the received message to
Returns:
			0 if the function ended witout errors, -1 if ended with errors, 2 if client disconnected gracefully
************************************************************************************************/
int receiveMessageProcess(char **stringToReceive, SOCKET *receiveSocket)
{
	TransferResult_t RecvRes;
	DWORD waitCode; BOOL releaseRes;
	RecvRes = ReceiveString(stringToReceive, *receiveSocket);

	if (RecvRes == TRNS_FAILED)
	{
		waitCode = WaitForSingleObject(logFileMutex, INFINITE); //raise log file mutex
		if (WAIT_OBJECT_0 != waitCode)
		{
			if (-1 == writeToLogFileAndPrint("Error when waiting for mutex", 1))
				return -1;
			exit(-1);
		}
		if (-1 == writeToLogFileAndPrint("can't receive message", 1))
			return -1;
		releaseRes = ReleaseMutex(logFileMutex); //release log file mutex
		if (releaseRes == FALSE)
		{
			if (-1 == writeToLogFileAndPrint("Error when releasing mutex", 1))
				return -1;
			exit(-1);
		}
		exit(-1);
	}
	else if (RecvRes == TRNS_DISCONNECTED)//gracefull disconnection of client
	{
		return 2;
	}
	return 0;
}

/************************************************************************************************
Description:
			this function update the array of playboard according to the play. receive messages 
			from client 'numOfClient' and send messages to clients accordingly
Parameters:
			input- int numOfClient: the number of client, 1 or 2, that played the current play
			output- none
Returns:
			int. 0 if succeeded, -1 if there were errors
************************************************************************************************/
int startPlayConnection(int numOfClient)
{
	TransferResult_t RecvResult, SendResult;
	char *receiveMessageString = NULL;
	char sendMessageString[LENGTH_OF_MESSAGE];
	SOCKET *currentSocketPtr=NULL, *otherSocketPtr=NULL;
	char checkStatus[LENGTH_OF_MESSAGE]; char endOfGameStatus[LENGTH_OF_MESSAGE]; char stringOfInt[4];
	int receivedMessageResult, otherClientNum, processReceivedMessageResult;

	chooseSockets(numOfClient, &currentSocketPtr, &otherSocketPtr, &otherClientNum);//choose sockets

	while (1 != isGameStartedFlag) {} //wait until the game starts

	if (-1 == sendStartOfGameMessages(numOfClient, currentSocketPtr, otherSocketPtr))//send start of game messages
		return -1; 

	//start playing
	while (1)
	{
		receiveMessageString = NULL;
		receivedMessageResult = receiveMessageProcess(&receiveMessageString, currentSocketPtr);//receive new message from client
		if (-1 == receivedMessageResult)
		{	
			exit(-1);
		}
		//exit
		else if (2 == receivedMessageResult) //client disconnected gracefully
		{
			if (-1 == writeToLogFileAndPrint("Player disconnected. Ending communication", 0))
				exit(-1);
			return 0;
		}
		//play
		processReceivedMessageResult = processReceivedMessage(&receiveMessageString); //process message
		if (1 == processReceivedMessageResult && numOfClient == turnFlag) //the client wants to play in his turn
		{
			if (0 == checkLegalPlay(numOfClient, atoi(receiveMessageString), &checkStatus)) //legal play
			{
				if (-1 == sendMessageProcess("PLAY_ACCEPTED", currentSocketPtr))
					return -1; //error
				updateBoard(numOfClient, atoi(receiveMessageString), boardArray); //update board
				if (1 == isEndGame(boardArray, &endOfGameStatus, numOfClient)) //game ended, message updated in endOfGameStatus
				{
					//send update to both clients
					if (-1 == sendMessageProcess(endOfGameStatus, currentSocketPtr) || -1 == sendMessageProcess(endOfGameStatus, otherSocketPtr))
						return -1; //error
					//end game 
					return 0;
				}
				else //game didn't end
				{
					if(-1 == gameContinueSendMessage(numOfClient, otherClientNum, receiveMessageString, currentSocketPtr, otherSocketPtr))
						return -1;
				}
			}
			else //illegal play, decline 
			{
				strcpy(sendMessageString, "PLAY_DECLINED:");
				strcat(sendMessageString, checkStatus);
				if (-1 == sendMessageProcess(sendMessageString, currentSocketPtr))
					return -1; //error
			}
		}
		else if(1 == processReceivedMessageResult && numOfClient != turnFlag) //the user wants to play not in his turn
		{ 
			checkLegalPlay(numOfClient, atoi(receiveMessageString), &checkStatus);
			strcpy(sendMessageString, "PLAY_DECLINED:");
			strcat(sendMessageString, checkStatus);
			if (-1 == sendMessageProcess(sendMessageString, currentSocketPtr))
				return -1; //error
		}
		//message
		else //its a send message type
		{
			if (-1 == transferMessageToOtherUser(numOfClient, receiveMessageString, otherSocketPtr))
				return -1;
		}
	}
	return 0;
}

/************************************************************************************************
Description:
			this function writes to log file using his mutex and print to screen
Parameters:
			input- char *stringToWrite: the string to write to the log file
				   int isError: 1 if it prints error, 0 if not
			output- none
Returns:
			1 if succeeded, -1 otherwise
************************************************************************************************/
int writeToLogFileAndPrint(char *stringToWrite, int isError)
{
	DWORD waitCode; BOOL releaseRes;

	waitCode = WaitForSingleObject(logFileMutex, INFINITE); //raise log file mutex
	if (WAIT_OBJECT_0 != waitCode)
	{
		printf("Error when waiting for mutex\n");
		fprintf(fPtrLog, "Custom message:Error when waiting for mutex\n");// printf to log file
		exit(-1);
	}
	printf("%s\n", stringToWrite); //print to screen
	if(1 == isError) //error print
		fprintf(fPtrLog, "Custom message: %s\n", stringToWrite);// printf to log file
	else if (0 == isError) //regular print
	{
		fprintf(fPtrLog, "%s\n", stringToWrite);// printf to log file
	}
	releaseRes = ReleaseMutex(logFileMutex); //release log file mutex
	if (releaseRes == FALSE)
	{
		printf("Error when releasing mutex\n");
		fprintf(fPtrLog, "Custom message:Error when releasing mutex\n");// printf to log file
		exit(-1);
	}
	return 0;
}

/************************************************************************************************
Description:
			this function update the type of message the server got and update the received string
			to contain only the message itself, without '\n'
Parameters:
			input- char **stringToReceive: pointer to a string contains the received message from client
			output- char **stringToReceive: pointer to a string contains the message itself from client, 
											cutting out the request type
Returns:
		    1=PLAY_REQUEST, 2=SEND_MESSAGE
************************************************************************************************/
int processReceivedMessage(char **stringToReceive)
{
	char *helperPtr = NULL;
	char requestTypeString[LENGTH_OF_MESSAGE];
	int typeOfRequest;

	helperPtr = strstr(*stringToReceive, ":");
	*helperPtr = '\0';
	helperPtr++;
	strcpy(requestTypeString, *stringToReceive); 
	if (0 == strcmp(requestTypeString, "PLAY_REQUEST"))
	{ 
		typeOfRequest = 1; 
	}
	if (0 == strcmp(requestTypeString, "SEND_MESSAGE"))
	{ 
		typeOfRequest = 2;
	}
	strcpy(*stringToReceive, helperPtr);
	if(1 == typeOfRequest ) //clear '\n'
	{ 
		helperPtr = strstr(*stringToReceive, "\n");
		*helperPtr = '\0';
	}
	return typeOfRequest;
}

/************************************************************************************************
Description:
			this function checks if the game has ended, and if ended with a draw or a win of one of the clients
			and update a send status accordingly
Parameters:
			input- int boardArray[BOARD_HEIGHT][BOARD_WIDTH]: the playboard array
				   int currentClientNum: the number of current client that made the move
			output- char **endOfGameStatus: pointer to string, updated accordingly if game ended: 1,2 or dead heat
Returns:
			1= game ended, 0= game has not ended
************************************************************************************************/
int isEndGame(int boardArray[BOARD_HEIGHT][BOARD_WIDTH], char **endOfGameStatus, int currentClientNum)
{
	char endOfGameStatusHelper[LENGTH_OF_MESSAGE];
	strcpy(endOfGameStatusHelper, "GAME_ENDED:");
	strcat(endOfGameStatusHelper, clientsUsernames[currentClientNum - 1]);//concatinate winner's username 

	// horizontalCheck 
	for (int j = 0; j < BOARD_WIDTH - 3; j++) {
		for (int i = 0; i < BOARD_HEIGHT; i++) {
			if ((boardArray[i][j] == currentClientNum && boardArray[i][j + 1] == currentClientNum) && (boardArray[i][j + 2] == currentClientNum && boardArray[i][j + 3] == currentClientNum)) {
				strcpy(endOfGameStatus, endOfGameStatusHelper);
				return 1;
			}
		}
	}
	// verticalCheck
	for (int i = 0; i < BOARD_HEIGHT - 3; i++) {
		for (int j = 0; j < BOARD_WIDTH; j++) {
			if ((boardArray[i][j] == currentClientNum && boardArray[i + 1][j] == currentClientNum) && (boardArray[i + 2][j] == currentClientNum && boardArray[i + 3][j] == currentClientNum)) {
				strcpy(endOfGameStatus, endOfGameStatusHelper);
				return 1;
			}
		}
	}
	// descendingDiagonalCheck 
	for (int i = 3; i < BOARD_HEIGHT; i++) {
		for (int j = 0; j < BOARD_WIDTH - 3; j++) {
			if ((boardArray[i][j] == currentClientNum && boardArray[i - 1][j + 1] == currentClientNum) && (boardArray[i - 2][j + 2] == currentClientNum && boardArray[i - 3][j + 3] == currentClientNum)) {
				strcpy(endOfGameStatus, endOfGameStatusHelper);
				return 1;
			}
		}
	}
	// ascendingDiagonalCheck
	for (int i = 3; i < BOARD_HEIGHT; i++) {
		for (int j = 3; j < BOARD_WIDTH; j++) {
			if ((boardArray[i][j] == currentClientNum && boardArray[i - 1][j - 1] == currentClientNum) && (boardArray[i - 2][j - 2] == currentClientNum && boardArray[i - 3][j - 3] == currentClientNum)) {
				strcpy(endOfGameStatus, endOfGameStatusHelper);
				return 1;
			}
		}
	}

	//check if board is full
	for (int i = 0; i < BOARD_HEIGHT; i++) {
		for (int j = 0; j < BOARD_WIDTH; j++) {
			if (0 == boardArray[i][j]){
				return 0; //not full, game not ended

			}
		}
	}

	strcpy(endOfGameStatus, "GAME_ENDED:dead_heat"); //game ended in a tie result
	return 1;
}

/************************************************************************************************
Description:
			this function shutdown the socket's connections
Parameters:
			input- none
			output- none
Returns:
			void
************************************************************************************************/
void shutdownSockets()
{
	shutdown(SockClient1, SD_SEND);
	shutdown(SockClient2, SD_SEND);
}

/************************************************************************************************
Description:
			this function choose the sockets in startPlayConnection function according to the 
			user's number 
Parameters:
			input- int numOfClient: the current client's number
			output- SOCKET **currentSocketPtr: pointer to pointer of the current socket
					SOCKET **otherSocketPtr: pointer to pointer to the other socket
					int *otherClientNum: pointer to the number of the other client
Returns:
			void
************************************************************************************************/
void chooseSockets(int numOfClient, SOCKET **currentSocketPtr, SOCKET **otherSocketPtr, int *otherClientNum)
{
	if (1 == numOfClient)
	{
		*currentSocketPtr = &SockClient1;
		*otherSocketPtr = &SockClient2;
		*otherClientNum = 2;
	}
	if (2 == numOfClient)
	{
		*currentSocketPtr = &SockClient2;
		*otherSocketPtr = &SockClient1;
		*otherClientNum = 1;
	}
}

/************************************************************************************************
Description:
			this function sends starting game messages to clients
Parameters:
			input- int numOfClient: the current client's number
				   SOCKET *currentSocketPtr:  pointer to the current socket's client
				   SOCKET *otherSocketPtr: pointer to the other socket's client
			output- none
Returns:
			0 if succeeded, -1 otherwise
************************************************************************************************/
int sendStartOfGameMessages(int numOfClient, SOCKET *currentSocketPtr, SOCKET *otherSocketPtr)
{
	char sendMessageString[LENGTH_OF_MESSAGE];

	if (1 == numOfClient)
	{
		if (-1 == sendMessageProcess("GAME_STARTED", currentSocketPtr) || -1 == sendMessageProcess("GAME_STARTED", otherSocketPtr))
			return -1; //error
		if (-1 == sendMessageProcess("BOARD_VIEW:8;8", currentSocketPtr) || -1 == sendMessageProcess("BOARD_VIEW:8;8", otherSocketPtr))
			return -1; //error
		strcpy(sendMessageString, "TURN_SWITCH:");
		strcat(sendMessageString, clientsUsernames[0]);
		if (-1 == sendMessageProcess(sendMessageString, currentSocketPtr) || -1 == sendMessageProcess(sendMessageString, otherSocketPtr))
			return -1; //error
	}
	return 0;
}

/************************************************************************************************
Description:
			this function transfers a message the server got from one client to the other client
Parameters:
			input- int numOfClient: the current client's number
				   char *receiveMessageString: the message to transfer to the other client
				   SOCKET *otherSocketPtr: pointer to the other socket's client
			output- none
Returns:
			0 if succeeded, -1 otherwise
************************************************************************************************/
int transferMessageToOtherUser(int numOfClient, char *receiveMessageString, SOCKET *otherSocketPtr)
{
	char sendMessageString[LENGTH_OF_MESSAGE];

	strcpy(sendMessageString, "RECEIVE_MESSAGE:");
	strcat(sendMessageString, clientsUsernames[numOfClient - 1]); //concatinate the client username who sent the message
	strcat(sendMessageString, ";");
	strcat(sendMessageString, receiveMessageString);
	if (-1 == sendMessageProcess(sendMessageString, otherSocketPtr)) //send message to other client
		return -1; //error
	return 0;
}

/************************************************************************************************
Description:
			this function sends messages after accepted play, to continue the game
Parameters:
			input- int numOfClient: the current client's number
				   int otherClientNum: the other client's number
				   char *receiveMessageString: the column to update according to this play made
				   SOCKET *currentSocketPtr: pointer to the current socket's client
				   SOCKET *otherSocketPtr: pointer to the other socket's client
			output- none
Returns:
			0 if succeeded, -1 otherwise
************************************************************************************************/
int gameContinueSendMessage(int numOfClient, int otherClientNum, char *receiveMessageString, SOCKET *currentSocketPtr, SOCKET *otherSocketPtr)
{
	char sendMessageString[LENGTH_OF_MESSAGE];
	char stringOfInt[4];

	strcpy(sendMessageString, "BOARD_VIEW:");
	_itoa(numOfClient, stringOfInt, 10);
	strcat(sendMessageString, stringOfInt); //concatinate the client number
	strcat(sendMessageString, ";");
	strcat(sendMessageString, receiveMessageString); //concatinate the column updated
	if (-1 == sendMessageProcess(sendMessageString, currentSocketPtr) || -1 == sendMessageProcess(sendMessageString, otherSocketPtr))
	{ 
		return -1; //error
	}
	//send that now is the other client turn
	strcpy(sendMessageString, "TURN_SWITCH:");
	strcat(sendMessageString, clientsUsernames[otherClientNum - 1]);
	if (-1 == sendMessageProcess(sendMessageString, currentSocketPtr) || -1 == sendMessageProcess(sendMessageString, otherSocketPtr))
		return -1; //error
	turnFlag = otherClientNum; //switch turn
	return 0;
}

/************************************************************************************************
Description:
			this function accept connections from third users and decline them
Parameters:
			input- SOCKET mainSocket: the socket that is listening to connections
			output- none
Returns:
			0 if succeeded, -1 otherwise
************************************************************************************************/
int declineOtherConnections(SOCKET mainSocket)
{
	SOCKET tempSocket,	AcceptSocket;

	while (1)
	{
		//accept connection 
		AcceptSocket = accept(mainSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET)
		{
			if (-1 == writeToLogFileAndPrint("Accepting connection with client failed, exiting", 1))
				return -1;
			exit(-1);
		}
		else //accept connection from client to server
		{
			tempSocket = AcceptSocket;// shallow copy: don't close AcceptSocket, instead close tempSocket when the time comes.
			if (-1 == sendMessageProcess("NEW_USER_DECLINED", &tempSocket)) //send to user that the request declined
				return -1;
			shutdown(tempSocket, SD_BOTH); //close connection
		}
	}
	return 0;
}
