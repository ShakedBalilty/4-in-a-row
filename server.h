/***
Project 4
Header file of server
***/

#pragma once
#define _CRT_SECURE_NO_WARNINGS  
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Windows.h>
#include <tchar.h>
#include <string.h>
#include <winsock2.h>
#include "SocketExampleShared.h"
#include "SocketSendRecvTools.h"

#define RED_PLAYER 1
#define YELLOW_PLAYER 2

#define BOARD_HEIGHT 6
#define BOARD_WIDTH  7

#define BLACK  15
#define RED    204
#define YELLOW 238

#define LENGTH_OF_MESSAGE 100
#define LENGTH_OF_USERNAME 30
#define NUM_OF_USERS 2

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~global variables declaration~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
extern int boardArray[BOARD_HEIGHT][BOARD_WIDTH];
extern int turnFlag; //who's turn is it now
extern int isGameStartedFlag;
extern FILE *fPtrLog;
extern HANDLE logFileMutex;
extern SOCKET SockClient1; 
extern SOCKET SockClient2;
extern char clientsUsernames[NUM_OF_USERS][LENGTH_OF_USERNAME]; 
extern HANDLE  hClient1Thread, hClient2Thread, hClient3Thread; //the handles for the three communication thread
extern int SERVER_PORT;
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int createThreadSimple(LPTHREAD_START_ROUTINE *StartAddress, VOID *Parameter, DWORD *ThreadId, HANDLE *h_thread); 
int checkLegalPlay(int playerNum, int columnPlay, char **checkStatus); 
int isEndGame(int boardArray[BOARD_HEIGHT][BOARD_WIDTH], char **endOfGameStatus, int currentClientNum);
int openServer(char *pathOfLogFile); 
int startPlayConnection(int numOfClient); 
int createThreads(); 
void shutdownSockets();
void chooseSockets(int numOfClient, SOCKET **currentSocketPtr, SOCKET **otherSocketPtr, int *otherClientNum);
int sendStartOfGameMessages(int numOfClient, SOCKET *currentSocketPtr, SOCKET *otherSocketPtr);
int transferMessageToOtherUser(int numOfClient, char *receiveMessageString, SOCKET *otherSocketPtr);
int gameContinueSendMessage(int numOfClient, int otherClientNum, char *receiveMessageString, SOCKET *currentSocketPtr, SOCKET *otherSocketPtr);
int declineOtherConnections(SOCKET mainSocket);
int registerUsername(int numOfClient, SOCKET *SockClient); 
int sendMessageProcess(char *stringToSend, SOCKET *sendSocket);
int receiveMessageProcess(char **stringToReceive, SOCKET *receiveSocket);
int processReceivedMessage(char **stringToReceive);
int writeToLogFileAndPrint(char *stringToWrite, int isError);


