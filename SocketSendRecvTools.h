/***
Project 4
Header file of socket send and recieve tools
***/
#ifndef SOCKET_SEND_RECV_TOOLS_H
#define SOCKET_SEND_RECV_TOOLS_H
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <stdio.h>
#include <string.h>

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

typedef enum { TRNS_FAILED, TRNS_DISCONNECTED, TRNS_SUCCEEDED } TransferResult_t;

TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd);
TransferResult_t SendString(const char *Str, SOCKET sd);
TransferResult_t ReceiveBuffer( char* OutputBuffer, int RemainingBytesToReceive, SOCKET sd);
TransferResult_t ReceiveString( char** OutputStrPtr, SOCKET sd);

/*******************************SHARED PRINT FUNCTIONS***********************************/
void updateBoard(int numOfClient, int updatedColumn, int boardArray[BOARD_HEIGHT][BOARD_WIDTH]);
void PrintBoard(int board[][BOARD_WIDTH], HANDLE consoleHandle);
void initializeBoardArray(int boardArray[BOARD_HEIGHT][BOARD_WIDTH]); 
/*****************************************************************************************/


#endif // SOCKET_SEND_RECV_TOOLS_H