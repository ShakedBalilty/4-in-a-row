# 4-in-a-row
Four in a row online game

START A NEW GAME
-
run server side:  
	```ex4.exe server <logfile path> <server port>```
  
run first client side:  
	```ex4.exe client <logfile path> <server port> <input_mode> <input file>```  
  
run second client side:  
	```ex4.exe client <logfile path> <server port> <input_mode> <input file>```
  
**note**: <input mode> argument on client side defines if this client is played by a human or from an input file.  
if you choose human: input_mode = human & leave the <input_file> argument empty.  
if you choose file: input_mode = file  & input file = path of the input file.  
  
  
HOW TO PLAY
-
each client (player) can use three commands:

	(1) play <column num>
	      column num: 1-7
	(2) message <message body>
	(3) exit
