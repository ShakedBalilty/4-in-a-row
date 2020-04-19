/***
Project 4
c file of Client
***/
#include "client.h"
#include "SocketExampleShared.h"
#include "SocketSendRecvTools.h"

/******************* global variables declaration******************************************************/
int myNumber = 0, SERVER_PORT, board[BOARD_HEIGHT][BOARD_WIDTH];
char myUserName[MAX_USERNAME_LENGTH];
BOOL myTurn = FALSE; // =TRUE if it is my turn
SOCKET m_socket; 
HANDLE  hConsole;
HANDLE hThread[3];
HANDLE bufferMutex = NULL;
HANDLE logMutex = NULL;
FILE *fPtrReadFile = NULL, *fPtrWriteResult = NULL;
messToSend *bufferHead = NULL;
/******************************************************************************************************/

/************************************************************************************************
Description:
			the function creates a mutex in order to synchronize the reading and writings from and to buffer.
Parameters:
			input- none.
			output- none.
Returns:
			int. 0 if the function succeeded. -1 if not.
*************************************************************************************************/

int createBufferMutex()
{
	bufferMutex = CreateMutex(NULL, FALSE, NULL);	// unnamed mutex 
	if (NULL == bufferMutex) //error occurred
	{
		printToLogFileAndScreen("Error when creating mutex", 1, 1);
		exit(ERROR_CODE); 
	}
}

/************************************************************************************************
Description:
			the function creates a mutex in order to synchronize the writings to the log file.
Parameters:
			input- none.
			output- none.
Returns:
			int. 0 if the function succeeded. -1 if not.
*************************************************************************************************/

int createLogMutex()
{
	logMutex = CreateMutex(NULL, FALSE, NULL);	// unnamed mutex 
	if (NULL == logMutex) //error occurred
	{
		printToLogFileAndScreen("Error when creating mutex", 1, 1);
		exit(ERROR_CODE); 
	}
}

/************************************************************************************************
Description:
			this function writes to log file using his mutex and print to screen.
Parameters:
			input- char *stringToWrite: the string to write to the log file
				   int custom: 1 if it is a custom message print, 0 if not
				   int mode: 1 if it is a print for screen, 2 for send message and 3 for received message
			output- none
Returns:
			1 if succeeded, -1 otherwise
************************************************************************************************/

int printToLogFileAndScreen(char *stringToWrite, int custom, int mode)
{
	DWORD waitCode; BOOL releaseRes;

	waitCode = WaitForSingleObject(logMutex, INFINITE); //raise log file mutex
	if (WAIT_OBJECT_0 != waitCode)
	{
		printf("Error when waiting for mutex\n");
		fprintf(fPtrWriteResult, "Custom message: Error when waiting for mutex\n");
		exit(ERROR_CODE); 
	}
	if (1 == mode)
		printf("%s\n", stringToWrite); //print to screen
	else if (2 == mode)
		fprintf(fPtrWriteResult, "Sent to server: ");// print to log file only in case it is a sent message
	else if (3 == mode)
		fprintf(fPtrWriteResult, "Received from server: ");// print to log file only in case it is a received message
	if (1 == custom)
		fprintf(fPtrWriteResult, "Custom message: ");// print to log file only in case it is a custom message
	if (NULL == strstr(stringToWrite, "\n"))
		fprintf(fPtrWriteResult, "%s\n", stringToWrite);// print to log file
	else 
		fprintf(fPtrWriteResult, "%s", stringToWrite);// print to log file
	releaseRes = ReleaseMutex(logMutex); //release log file mutex
	if (releaseRes == FALSE)
	{
		printf("Error when releasing mutex\n");
		fprintf(fPtrWriteResult, "Custom message: Error when releasing mutex\n");
		exit(ERROR_CODE);
	}
	return SUCCESS_CODE;
}


/************************************************************************************************
Description:
			the function creates new message in struct of messToSend.
Parameters:
			input- char *message: the content.
			output- none.
Returns:
			messToSend*: the new message.
*************************************************************************************************/
messToSend* createMessToSend(char *message)
{
	messToSend *newMess;
	newMess = (messToSend*)malloc(sizeof(messToSend));
	if (NULL == newMess)
	{
		printToLogFileAndScreen("Fatal error: memory allocation failed!", 1, 1);
		exit(ERROR_CODE);
	}
	strcpy(newMess->message, message);
	newMess->next = NULL;
	return newMess;
}

/************************************************************************************************
Description:
			the function gets a string and checks if it represent a number. the function ignores the '\n'.
Parameters:
			input- char *string: the string to be checked.
			output- none.
Returns:
			int. 0 if the the string represent a number. -1 if not.
*************************************************************************************************/

int isNumber(char *string)
{
	int length = 0, i = 0;
	length = strlen(string);
	for (i = 0; i < length; i++)
		if (!isdigit(string[i]))
			return ERROR_CODE;
	return SUCCESS_CODE;
}

/************************************************************************************************
Description:
			the function checks if the message is valid, and if so creates a new messsage in the
			required format in order to send it to the server.
Parameters:
			input- char* message: the message to check.
			output- char *newMessage: the message to the server.
Returns:
			int. 1 if the message is exit, 2 if the message is play, 3 if the message is for chat
			and -1 if the message is invalid.
*************************************************************************************************/
int checkValidMessage(char *message, char *newMessage)
{
	int ind = 8, newInd = 13;
	char *helperPtr1 = NULL;

	if (0 == strcmp(message, "message "))  //empty message
		return SUCCESS_CHAT;
	if (0 == strncmp(message, "message ", 8))  //send_message
	{
		strcpy(newMessage, "SEND_MESSAGE");
		newMessage[12] = ':';
		while (ind < strlen(message))
		{
			if ((' ' == message[ind]) && (' ' != message[ind - 1]))
			{
				newMessage[newInd] = ';';
				newMessage[newInd + 1] = ' ';
				newMessage[newInd + 2] = ';';
				ind++;
				newInd = newInd + 3;
				continue;
			}
			if (' ' == message[ind])
			{
				newMessage[newInd] = ' ';
				newMessage[newInd + 1] = ';';
				ind++;
				newInd = newInd + 2;
				continue;
			}
			newMessage[newInd] = message[ind];
			ind++;
			newInd++;
		}
		newMessage[newInd] = '\n';  
		newMessage[newInd + 1] = '\0';
		return SUCCESS_CHAT;
	}
	if (0 == strncmp(message, "play ", 5))  // play_request
	{
		strcpy(newMessage, "PLAY_REQUEST:");
		helperPtr1 = strstr(message, " "); //find the first occurence of " " - necessarily exists
		if (0 == isNumber(helperPtr1 + 1))
		{
			strcat(newMessage, helperPtr1 + 1);
			strcat(newMessage, "\n");          
			return SUCCESS_PLAY;
		}
		return ERROR_CODE;
	}
	if (0 == strcmp(message, "exit"))  // exit_request
	{
		return EXIT_CODE;
	}
	return ERROR_CODE;
}


/************************************************************************************************
Description:
			the function add the new message to the end of the buffer.
Parameters:
			input- messToSend *newMess: the message to be added.
			output- none.
Returns:
			none.
*************************************************************************************************/

void addMessToBuffer(messToSend *newMess)
{
	messToSend *copyHead = bufferHead;
	if (NULL == bufferHead) //the buffer is empty
	{
		bufferHead = newMess;
		return;
	}
	while (copyHead->next != NULL)
		copyHead = copyHead->next;
	copyHead->next = newMess; //add new item to the end of the buffer
	return;
}

/************************************************************************************************
Description:
			the function deletes the first message of the buffer.
Parameters:
			input- none.
			output- messToSend *bufferHead: global pointer becomes the next message in buffer.
Returns:
			none.
*************************************************************************************************/
void deleteMessfromBuffer()
{
	messToSend *copyHead = bufferHead;
	if (NULL != bufferHead)
		bufferHead = bufferHead->next; //delete first message in buffer
	free(copyHead);
}


/************************************************************************************************
Description:
			the function checks the kind of mode according to the input file argument.
Parameters:
			input- char *charInputMode: the input mode argument.
			output- none.
Returns:
			int. 1 if we are in file mode, 0 otherwise.
*************************************************************************************************/

int getUserMode(char *inputMode)
{
	if (0 == strcmp(inputMode, "file")) //file mode
		return 1;
	return 0; //human mode
}


/************************************************************************************************
Description:
			the function manage the reception of the messages from the keybord in case we are in human mode.
Parameters:
			input- none.
			output- none.
Returns:
			none
*************************************************************************************************/

void runHumanMode()
{
	int isValidMess = 0, firstLine = 1;
	char userName[MAX_USERNAME_LENGTH], sendStr[MAX_MESSAGEE_LENGTH] = "NEW_USER_REQUEST:";
	char newString[MAX_MESSAGEE_LENGTH] = "";
	TransferResult_t sendRes;
	messToSend *newMess = NULL;
	DWORD wait_code;
	BOOL returnedVal;

	printf("Hello dear guest, please choose a username.\nInput: ");
	while (1)
	{
		if (1 == firstLine)
		{
			gets(userName, sizeof(userName)); //Reading a user name from the keyboard
			strcat(sendStr, userName);
			strcat(sendStr, "\n");
			newMess = createMessToSend(sendStr);
			addMessToBuffer(newMess);
			firstLine = 0;
			continue;  
		}
		gets(sendStr, sizeof(sendStr)); //Reading a string from the keyboard
		isValidMess = checkValidMessage(sendStr, newString);
		if (ERROR_CODE == isValidMess) //illegal message 
		{
			printToLogFileAndScreen("Error: Illegal command", 0, 1);
			continue;
		}
		else if (EXIT_CODE == isValidMess) //the player ask to exit from the game
		{
			while (NULL != bufferHead) //ensure all the messages in buffer will be send
			{ }
			return(EXIT_CODE); //should end the program and close everything
		}	
		newMess = createMessToSend(newString); //otherwise- play request or chat message
		if ((NULL != bufferHead) && (NULL == bufferHead->next)) //buffer has only one message
		{
			wait_code = WaitForSingleObject(bufferMutex, INFINITE); //raise buffer mutex
			if (WAIT_OBJECT_0 != wait_code)
			{
				printToLogFileAndScreen("Error when waiting for mutex", 1, 1);
				exit(ERROR_CODE);
			}
			addMessToBuffer(newMess);
			returnedVal = ReleaseMutex(bufferMutex); //release buffer mutex
			if (FALSE == returnedVal)
			{
				printToLogFileAndScreen("Error when releasing mutex", 1, 1);
				exit(ERROR_CODE);
			}
		}
		else
			addMessToBuffer(newMess);
	}
}


/************************************************************************************************
Description:
			the function manage the reception of the messages from the file in case we are in file mode.
Parameters:
			input- none.
			output- none.
Returns:
			none
*************************************************************************************************/

int runFileMode()
{
	int isValidMess = 0, firstLine = 1;
	char sendStr[MAX_MESSAGEE_LENGTH] = "NEW_USER_REQUEST:";
	char newString[MAX_MESSAGEE_LENGTH] = "", *helperPtr = NULL;
	TransferResult_t sendRes;
	messToSend *newMess = NULL;
	DWORD wait_code;
	BOOL returnedVal;

	while (!feof(fPtrReadFile))
	{
		if (1 == firstLine)
		{
			fgets(myUserName, MAX_USERNAME_LENGTH, fPtrReadFile);
			strcat(sendStr, myUserName);
			helperPtr = strstr(myUserName, "\n");
			if (NULL != helperPtr)
				*helperPtr = '\0';
			newMess = createMessToSend(sendStr);
			addMessToBuffer(newMess);
			firstLine = 0;
			continue;
		}
		fgets(sendStr, MAX_MESSAGEE_LENGTH, fPtrReadFile);
		helperPtr = strstr(sendStr, "\n");
		if (NULL != helperPtr)
			*helperPtr = '\0';
		isValidMess = checkValidMessage(sendStr, newString);
		if (ERROR_CODE == isValidMess)
		{
			printToLogFileAndScreen("Error: Illegal command", 0, 1);
			continue;
		}
		else if (EXIT_CODE == isValidMess)
		{
			while (NULL != bufferHead)
			{
				continue;
			}
			return(EXIT_CODE); //should end the program and close everything
		}
		newMess = createMessToSend(newString); //otherwise- play request or chat message
		if (SUCCESS_PLAY == isValidMess)
		{
			while (FALSE == myTurn )  //wait until turn switch
			{ }
		}
		myTurn = FALSE;
		if ((NULL != bufferHead) && (NULL == bufferHead->next))
		{
			wait_code = WaitForSingleObject(bufferMutex, INFINITE); //raise buffer mutex
			if (WAIT_OBJECT_0 != wait_code)
			{
				printToLogFileAndScreen("Error when waiting for mutex", 1, 1);
				exit(ERROR_CODE);
			}
			addMessToBuffer(newMess); //add message to buffer in case it has only one message
			returnedVal = ReleaseMutex(bufferMutex); //release buffer mutex
			if (FALSE == returnedVal)
			{
				printToLogFileAndScreen("Error when releasing mutex", 1, 1);
				exit(ERROR_CODE);
			}
		}
		else
			addMessToBuffer(newMess); //add message to buffer in case it has more than one message
	}
	fclose(fPtrReadFile); //close log file
	fPtrReadFile = NULL;
}


/************************************************************************************************
Description:
			the function manage the reception of the messages from the keybord or from a file.
Parameters:
			input- int inputMode: 0 for human mode, 1 for file mode.
			output- none.
Returns:
			0 when the thread is finished.
*************************************************************************************************/

DWORD userInterfaceThreadRoutine(int inputMode)
{
	if (0 == inputMode) //human mode
		runHumanMode();
	else               //file mode
		runFileMode();
	return 0;
}

/************************************************************************************************
Description:
			the function reads data coming from the server
Parameters:
			input- none.
			output- none.
Returns:
			DWORD. 0 there were no errors.
*************************************************************************************************/

DWORD receiveThreadRoutine(void)
{
	TransferResult_t RecvRes;

	while (1)
	{
		char *AcceptedStr = NULL;

		RecvRes = ReceiveString(&AcceptedStr, m_socket);
		if (RecvRes == TRNS_FAILED)
		{
			printToLogFileAndScreen("Socket error while trying to write data to socket", 1, 1);
			exit(ERROR_CODE); //return 0x555;
		}
		else if (RecvRes == TRNS_DISCONNECTED) //Server closed connection
		{
			printToLogFileAndScreen("Server disconnected. Exiting.", 0, 1);
			return TRNS_DISCONNECTED;
		}
		else
		{
			printToLogFileAndScreen(AcceptedStr, 0, 3);  //prints the server's message
			responseToMess (AcceptedStr);               //respond to the server's message
		}
		free(AcceptedStr);
	}
	return 0;
}

/************************************************************************************************
Description:
			the function send data to the server
Parameters:
			input- messToSend *bufferHead: the global head of buffer where the message is sent.
			output- none.
Returns:
			DWORD. 0 there were not errors.
*************************************************************************************************/

DWORD sendThreadRoutine()
{
	char SendStr[MAX_MESSAGEE_LENGTH];
	TransferResult_t SendRes;
	DWORD wait_code;
	BOOL returnedVal;

	while (1)
	{
		if (NULL != bufferHead) { //there are messages to send
			if (NULL == bufferHead->next)  //the buffer has only one item
			{
				wait_code = WaitForSingleObject(bufferMutex, INFINITE); //raise buffer mutex
				if (WAIT_OBJECT_0 != wait_code)
				{
					printToLogFileAndScreen("Error when waiting for mutex", 1, 1);
					exit(ERROR_CODE);
				}
				SendRes = SendString(bufferHead->message, m_socket); //send message from buffer in case it has only one message
				if (SendRes == TRNS_FAILED)
				{
					printToLogFileAndScreen("Socket error while trying to write data to socket", 1, 1);
					exit(ERROR_CODE);
				}
				printToLogFileAndScreen(bufferHead->message, 0, 2);
				deleteMessfromBuffer(bufferHead);
				returnedVal = ReleaseMutex(bufferMutex); //release buffer mutex
				if (returnedVal == FALSE)
				{
					printToLogFileAndScreen("Error when releasing mutex", 1, 1);
					exit(ERROR_CODE);
				}
			}
			else  //the buffer has more than one item
			{
				SendRes = SendString(bufferHead->message, m_socket); //send message from buffer in case it has more than on
				if (SendRes == TRNS_FAILED)
				{
					printToLogFileAndScreen("Socket error while trying to write data to socket", 1, 1);
					exit(ERROR_CODE);
				}
				printToLogFileAndScreen(bufferHead->message, 0, 2);
				deleteMessfromBuffer(bufferHead);           //delete message after it was sent
			}
		}
	} 
	return 0;
}


/************************************************************************************************
Description:
			the function gets a chat message from the other player and prints it to the screen.
Parameters:
			input- char *ReceivedMessage: the message from the other player.
			output- none.
Returns:
			none.
*************************************************************************************************/

void printReceivedMess(char *receivedMessage)
{
	char messToPrint[110] = " "; 
	int ind = 0, currentInd = 0, length = strlen(receivedMessage);

	while (receivedMessage[ind] != ':')
	{
		ind++;
	}
	ind++;
	if (NULL != strstr(receivedMessage, ";"))
	{
		while (receivedMessage[ind] != ';')
		{
			messToPrint[currentInd] = receivedMessage[ind];
			ind++;
			currentInd++;
		}
	}	
	messToPrint[currentInd] = ':';
	currentInd++;
	messToPrint[currentInd] = ' ';
	currentInd++;
	ind++;
	for (ind; ind < length; ind++)
	{
		if (receivedMessage[ind] != ';')
		{
			messToPrint[currentInd] = receivedMessage[ind];
			currentInd++;
		}
	}
	messToPrint[currentInd + 1] = '\0';
	printf("%s", messToPrint);
}

/************************************************************************************************
Description:
			the function gets a play declined from the server and prints it to the screen.
Parameters:
			input- char *ReceivedMessage: the message from the other player.
			output- none.
Returns:
			none.
*************************************************************************************************/
void printPlayDeclined(char *receivedMessage)
{
	char messToPrint[110] = " ";
	int ind = 0, currentInd = 0, length = strlen(receivedMessage);
	while (receivedMessage[ind] != ':')
	{
		ind++;
	}
	ind++;
	for (ind; ind < length; ind++)
	{
		if (receivedMessage[ind] != ';')
		{
			messToPrint[currentInd] = receivedMessage[ind];
			currentInd++;
		}
	}
	messToPrint[currentInd + 1] = '\0';
	printf("%s\n", messToPrint);
}

/************************************************************************************************
Description:
			the function checks which kind of message the server sent. The function prints to the screen
			and to the log fie if needed.
Parameters:
			input- char *AcceptedStr: the message from the server.
			output- none.
Returns:
			none.
*************************************************************************************************/

void responseToMess(char *AcceptedStr)
{
	int numOfPlayer = 0, updatedColumn = 0;
	char *userName = NULL, turn[MAX_USERNAME_LENGTH +10]="", playerChar = ' ', columnChar = ' ';

	if (0 == strncmp(AcceptedStr, "NEW_USER_DECLINED", 17))  //server sent "NEW_USER_ACCEPTED"
	{
		printf("Request to join was refused");
		endOfProgram(); //WSACleanup() and close all resources
		return;
	}
	if (0 == strncmp(AcceptedStr, "NEW_USER_ACCEPTED", 17))  //server sent "NEW_USER_ACCEPTED"
	{
		numOfPlayer = strstr(AcceptedStr, ":"); //find the appearance of :
		myNumber = atoi(numOfPlayer + 1);
		return;
	}
	else if (0 == strcmp(AcceptedStr, "GAME_STARTED"))  //server sent "GAME_STARTED"
	{
		printf("Game is on!\n");
		return;
	}
	else if (0 == strncmp(AcceptedStr, "TURN_SWITCH", 11))  //server sent "TURN_SWITCH"
	{
		userName = strstr(AcceptedStr, ":");
		if (0 == strcmp(myUserName, userName + 1))
			myTurn = TRUE;
		strcpy(turn, userName + 1);
		strcat(turn, "'s turn");
		printToLogFileAndScreen(turn, 0, 1);
		return;
	}
	else if (0 == strncmp(AcceptedStr, "BOARD_VIEW", 10))  //server sent "BOARD_VIEW"
	{
		playerChar = AcceptedStr[11]; //the number of player
		columnChar = AcceptedStr[13]; //the updated column
		numOfPlayer = playerChar - '0';
		updatedColumn = columnChar - '0';
		updateBoard(numOfPlayer, updatedColumn, board);
		PrintBoard(board, hConsole);
		return;
	}
	else if (0 == strncmp(AcceptedStr, "RECEIVE_MESSAGE", 15))   //server sent "RECEIVE_MESSAGE"
	{
		printReceivedMess(AcceptedStr);
		return;
	}
	else if (0 == strncmp(AcceptedStr, "PLAY_DECLINED", 13))   //server sent "RECEIVE_MESSAGE"
	{
		if (NULL != strstr(AcceptedStr, "Illegal"))
			myTurn = TRUE;
		printPlayDeclined(AcceptedStr);
		return;
	}
	else if (0 == strcmp(AcceptedStr, "GAME_ENDED:dead_heat"))  //server sent "GAME_ENDED" and no one won
	{
		printToLogFileAndScreen("Game ended. Everybody wins!", 0, 1);
		endOfProgram(); //WSACleanup() and close all resources
		return;
	}
	else if (0 == strncmp(AcceptedStr, "GAME_ENDED",10))   //server sent "GAME_ENDED" and someone won
	{
		userName = strstr(AcceptedStr, ":");
		strcpy(turn, "Game ended. The winner is ");
		strcat(turn, userName+1);
		strcat(turn, "!");
		printToLogFileAndScreen(turn, 0, 1);
		endOfProgram(); //WSACleanup() and close all resources
		return;
	}
}

/************************************************************************************************
Description:
			The function free all the dynamic memory used for buffer.

Parameters:
			input- none.
			output- none.
Returns:
			none.
*************************************************************************************************/
void freeBuffer()
{
	messToSend* temp;
	while (bufferHead != NULL) //as long as buffer still contains messages
	{
		temp = bufferHead;
		bufferHead = bufferHead->next;
		free(temp);
	}
}

/************************************************************************************************
Description:
			The function finishes the program- releases dynamic memory and closes threads and resources

Parameters:
			input- none.
			output- none.
Returns:
			none.
*************************************************************************************************/
int endOfProgram()
{
	DWORD lpExitCode;

	if (NULL != bufferHead)
		freeBuffer(bufferHead);
	if (NULL != fPtrReadFile) //if it was a file mode
		fclose(fPtrReadFile); //close input file
	fclose(fPtrWriteResult); //close log file
	if (0 != GetExitCodeThread(hThread[0], &lpExitCode))
	{
		if (STILL_ACTIVE == lpExitCode)
			TerminateThread(hThread[0], 0x555);
	}
	//close all threads and handles
	CloseHandle(hThread[0]);
	TerminateThread(hThread[1], 0x555);
	CloseHandle(hThread[1]);
	TerminateThread(hThread[2], 0x555);
	CloseHandle(hThread[2]);
	shutdown(m_socket, SD_SEND);
	CloseHandle(hConsole);
	CloseHandle(bufferMutex);
	CloseHandle(logMutex);
	closesocket(m_socket);
	WSACleanup();
	return SUCCESS_CODE;
}

/************************************************************************************************
Description:
			This is the main function for running as a client. It is responsible for creating communication
			with the server and for opening the threads of the user interface, sending and receiving.
Parameters:
			input- int inputMode: the input mode argument (0 for human and 1 for file), char *logFilePath: the path
			of the log file and char* inputFilePath: the path of the file in case we are in file mode.
			output- none.
Returns:
			none.
*************************************************************************************************/

void mainClient(int inputMode, char *logFilePath, char* inputFilePath)
{
	SOCKADDR_IN clientService;
	int iResult = 0;
	char connectingFailed[200] = "Failed connecting to server on 127.0.0.1:";
	char connectingSucceeded[200] = "Connected to server on 127.0.0.1:";
	char portNum[10];
	
	initializeBoardArray(board);
	_itoa(SERVER_PORT, portNum, 10);
	// Initialize Winsock.
	WSADATA wsaData; //Create a WSADATA object called wsaData.
	//The WSADATA structure contains information about the Windows Sockets implementation.
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	createBufferMutex();  // create buffer mutex
	createLogMutex(); // create log mutex
	fPtrWriteResult = fopen(logFilePath, "w"); //open log file to write
	if (fPtrWriteResult == NULL) {
		printf("An error occurred during execution, couldn’t open the log file!/n");
		exit (ERROR_CODE);    //writing failed
	}
	//Call WSAStartup and check for errors.
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		printToLogFileAndScreen("WSAStartup failed", 1, 1);
		exit(ERROR_CODE);
	}
	//Create a socket- Call the socket function and return its value to the m_socket variable.
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// Check for errors to ensure that the socket is a valid socket.
	if (m_socket == INVALID_SOCKET) {  // If the socket call fails, it returns INVALID_SOCKET.
		printToLogFileAndScreen("Error at socket", 1, 1);
		WSACleanup();
		exit(ERROR_CODE);
	}

	 //Create a sockaddr_in object clientService and set values.
	clientService.sin_family = AF_INET;  //AF_INET is the Internet address family.
	clientService.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); //Setting the IP address to connect to
	clientService.sin_port = htons(SERVER_PORT); //Setting the port to connect to.
	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	if (connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
		strcat(connectingFailed, portNum);
		strcat(connectingFailed, ". Exiting");
		printToLogFileAndScreen(connectingFailed, 0, 1);
		exit(ERROR_CODE);  
	}
	strcat(connectingSucceeded, portNum);
	printToLogFileAndScreen(connectingSucceeded, 0, 1);
	if (1 == inputMode) //file mode
	{
		fPtrReadFile = fopen(inputFilePath, "r");   //open the file to read from
		if (fPtrReadFile == NULL) {
			printToLogFileAndScreen("An error occurred during execution, couldn’t open the file!", 1, 1);
			exit(ERROR_CODE);    //writing failed
		}
	}
	// Send and receive data.
	hThread[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)userInterfaceThreadRoutine, inputMode, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)sendThreadRoutine, NULL, 0, NULL);
	hThread[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveThreadRoutine, NULL, 0, NULL);
	WaitForMultipleObjects(3, hThread, FALSE, INFINITE); //in case that one or more of the threads is finished
	endOfProgram(); //WSACleanup() and close all resources
	return EXIT_CODE;
}





