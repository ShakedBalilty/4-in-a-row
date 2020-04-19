/***
Project 4
Header file of client
***/
#pragma once
#define _CRT_SECURE_NO_WARNINGS  
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>
#include <ctype.h>
#include "SocketExampleShared.h"
#include "SocketSendRecvTools.h"

#define ERROR_CODE ((int)(-1))
#define SUCCESS_CODE ((int)(0))
#define EXIT_CODE ((int)(1))
#define SUCCESS_PLAY ((int)(2))
#define SUCCESS_CHAT ((int)(3))
#define MAX_USERNAME_LENGTH 32
#define MAX_MESSAGEE_LENGTH 102
#define RED_PLAYER 1
#define YELLOW_PLAYER 2
#define BOARD_HEIGHT 6
#define BOARD_WIDTH  7
#define BLACK  15
#define RED    204
#define YELLOW 238


typedef struct  MessToSend {
	char message[MAX_MESSAGEE_LENGTH]; //longest message is 100 chars (content) + "\n\0"
	struct MessToSend *next; // the next message in the linked list
} messToSend;

/* global variables declaration*/
extern int myNumber, board[BOARD_HEIGHT][BOARD_WIDTH], SERVER_PORT;
extern char myUserName[MAX_USERNAME_LENGTH];
extern BOOL myTurn;
extern SOCKET m_socket;
extern HANDLE  hConsole;
extern HANDLE hThread[3];
extern HANDLE bufferMutex;
extern HANDLE logMutex;
extern FILE *fPtrReadFile, *fPtrWriteResult;
extern messToSend *bufferHead;
/***********************************************************************************************/
int createBufferMutex(); 
int createLogMutex(); 
int printToLogFileAndScreen(char *stringToWrite, int custom, int screen); 
messToSend* createMessToSend(char *message); 
int isNumber(char *string);
int checkValidMessage(char *message, char *newMessage); 
void addMessToBuffer(messToSend *newMess); 
void deleteMessfromBuffer(); 
int getUserMode(char *inputMode); 
void runHumanMode();
int runFileMode();
DWORD userInterfaceThreadRoutine(int inputMode);
DWORD receiveThreadRoutine(void); 
DWORD sendThreadRoutine(); 
void printReceivedMess(char *receivedMessage); 
void printPlayDeclined(char *receivedMessage); 
void responseToMess(char *AcceptedStr); 
void freeBuffer();
int endOfProgram();
void mainClient(int inputMode, char *logFilePath, char* inputFilePath); 


