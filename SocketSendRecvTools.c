/***
Project 4
c file of socket send and recieve tools
***/
#include "SocketSendRecvTools.h"
#include <stdio.h>
#include <string.h>

/************************************************************************************************
Description:
			the function uses a socket to send a buffer.
Parameters:
			input- Buffer: the buffer containing the data to be sent, BytesToSend: the number of bytes
			from the Buffer to send, sd: the socket used for communication.
			output- none.
Returns:
			TransferResult_t. TRNS_SUCCEEDED - if sending succeeded, TRNS_FAILED - otherwise.
*************************************************************************************************/

TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd)
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;

	while (RemainingBytesToSend > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesTransferred = send(sd, CurPlacePtr, RemainingBytesToSend, 0);
		if (BytesTransferred == SOCKET_ERROR)
			return TRNS_FAILED;
		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred; // <ISP> pointer arithmetic
	}
	return TRNS_SUCCEEDED;
}

/************************************************************************************************
Description:
			the function uses a socket to send a string.
Parameters:
			input- Str: the string to send,  sd: the socket used for communication.
			output- none.
Returns:
			TransferResult_t. TRNS_SUCCEEDED - if sending succeeded, TRNS_FAILED or TRNS_DISCONNECTED- otherwise.
*************************************************************************************************/

TransferResult_t SendString(const char *Str, SOCKET sd)
{
	/* Send the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	TransferResult_t SendRes;

	/* The request is sent in two parts. First the Length of the string (stored in
	   an int variable ), then the string itself. */

	TotalStringSizeInBytes = (int)(strlen(Str) + 1); // terminating zero also sent	

	SendRes = SendBuffer(
		(const char *)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)), // sizeof(int) 
		sd);

	if (SendRes != TRNS_SUCCEEDED) return SendRes;

	SendRes = SendBuffer(
		(const char *)(Str),
		(int)(TotalStringSizeInBytes),
		sd);

	return SendRes;
}

/************************************************************************************************
Description:
			the function uses a socket to receive a buffer.
Parameters:
			input- OutputBuffer: pointer to a buffer into which data will be written, OutputBufferSize: size
			in bytes of Output Buffer, BytesReceivedPtr: output parameter. if function returns TRNS_SUCCEEDED, then this
 			will point at an int containing the number of bytes received,  sd: the socket used for communication.
			output- none.
Returns:
			TransferResult_t. TRNS_SUCCEEDED - if receiving succeeded,TRNS_DISCONNECTED - if the socket was
			disconnected and TRNS_FAILED - otherwise.
*************************************************************************************************/

TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd)
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;

	while (RemainingBytesToReceive > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);
		if (BytesJustTransferred == SOCKET_ERROR)
			return TRNS_FAILED;
		else if (BytesJustTransferred == 0)
			return TRNS_DISCONNECTED; // recv() returns zero if connection was gracefully disconnected.

		RemainingBytesToReceive -= BytesJustTransferred;
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

/************************************************************************************************
Description:
			the function uses a socket to receive a string, and stores it in dynamic memory.
Parameters:
			input- OutputStrPtr: a pointer to a char-pointer that is initialized to NULL, 
			sd: the socket used for communication.
			output- none.
Returns:
			TransferResult_t. TRNS_SUCCEEDED - if receiving succeeded,TRNS_DISCONNECTED - if the socket was
			disconnected and TRNS_FAILED - otherwise.
*************************************************************************************************/

TransferResult_t ReceiveString(char** OutputStrPtr, SOCKET sd)
{
	/* Recv the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	TransferResult_t RecvRes;
	char* StrBuffer = NULL;

	if ((OutputStrPtr == NULL) || (*OutputStrPtr != NULL))
	{
		printf("The first input to ReceiveString() must be "
			"a pointer to a char pointer that is initialized to NULL. For example:\n"
			"\tchar* Buffer = NULL;\n"
			"\tReceiveString( &Buffer, ___ )\n");                               
		return TRNS_FAILED;
	}

	/* The request is received in two parts. First the Length of the string (stored in
	   an int variable ), then the string itself. */

	RecvRes = ReceiveBuffer(
		(char *)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)), // 4 bytes
		sd);

	if (RecvRes != TRNS_SUCCEEDED) return RecvRes;

	StrBuffer = (char*)malloc(TotalStringSizeInBytes * sizeof(char));

	if (StrBuffer == NULL)
		return TRNS_FAILED;

	RecvRes = ReceiveBuffer(
		(char *)(StrBuffer),
		(int)(TotalStringSizeInBytes),
		sd);

	if (RecvRes == TRNS_SUCCEEDED)
	{
		*OutputStrPtr = StrBuffer;
	}
	else
	{
		free(StrBuffer);
	}

	return RecvRes;
}

/**************************shared print functions***********************************/
/***********************************************************
* This function prints the board, and uses O as the holes.
* The disks are presented by red or yellow backgrounds.
* code was changed so that the buttom line is 0 and the upper line is 6
* Input: A 2D array representing the board and the console handle
* Output: Prints the board, no return value
************************************************************/
void PrintBoard(int board[][BOARD_WIDTH], HANDLE consoleHandle)
{

	int row, column;
	//Draw the board
	for (row = BOARD_HEIGHT - 1; row >= 0; row--)
	{
		for (column = 0; column < BOARD_WIDTH; column++)
		{
			printf("| ");
			if (board[row][column] == RED_PLAYER)
				SetConsoleTextAttribute(consoleHandle, RED);

			else if (board[row][column] == YELLOW_PLAYER)
				SetConsoleTextAttribute(consoleHandle, YELLOW);

			printf("O");

			SetConsoleTextAttribute(consoleHandle, BLACK);
			printf(" ");
		}
		printf("\n");

		//Draw dividing line between the rows
		for (column = 0; column < BOARD_WIDTH; column++)
		{
			printf("----");
		}
		printf("\n");
	}


}

/************************************************************************************************
Description:
			this function update the array of playboard according to the play
Parameters:
			input- int numOfClient: the number of client, 1 or 2, that played the current play
				   int updatedColumn: the column to update in the playboard array
			output- int boardArray[NUM_OF_USERS][BOARD_WIDTH]: the playboard array, its size is BOARD_HEIGHT x BOARD_WIDTH
Returns:
			void
************************************************************************************************/
void updateBoard(int numOfClient, int updatedColumn, int boardArray[BOARD_HEIGHT][BOARD_WIDTH])
{
	int height = BOARD_HEIGHT - 1;
	if ((8 == numOfClient) && (8 == updatedColumn)) //update for the start of the game
		return;
	else
	{ 
		while ((0 == boardArray[height][updatedColumn-1]) && (-1 != height))
		{
			height= height-1;
		}
		boardArray[height + 1][updatedColumn-1] = numOfClient;
	}
}

/************************************************************************************************
Description:
			this function initialize the array that represent the playboard to zeroes
Parameters:
			input- none
			output- int boardArray[NUM_OF_USERS][BOARD_WIDTH]: the playboard array, its size is BOARD_HEIGHT x BOARD_WIDTH
Returns:
			void
************************************************************************************************/
void initializeBoardArray(int boardArray[BOARD_HEIGHT][BOARD_WIDTH])
{
	int row = 0, column = 0;

	for (row = 0; row < BOARD_HEIGHT; row++)
	{
		for (column = 0; column < BOARD_WIDTH; column++)
		{
			boardArray[row][column] = 0;
		}
	}
}