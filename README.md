# banking-system
Banking system created for Linux for a school project

Banking Client/Server Documentation
12/9/2018

[--USAGE--]
Run:
make clean
make
to compile all code. Then, run
./bankingServer <port>
./bankingClient <hostname> <port> (on another system)
Commands: create <string accountName>, serve <string accountName>, deposit <double amount>,
withdraw <double amount>, query, end, quit

[--DESIGN DOCUMENTATION--]
SERVER
The first thing that occurs is the setup for the 15 second diagnostic timer. Then it uses
a new socket to connect to any client. Upon the first connection, it enters an infinite loop
which continues to detect and accrue connections. Each thread runs the function clientThread().
This function listens for commands from the client and runs the proper checks and code. To
detect the command, a destination string is set to null terminators, then the exact amount
of characters for each command is copied over and tested. When SIGINT is detected, all
pthreads are canceled, all sockets are closed, and the linked list is destroyed.

CLIENT
Upon startup, the client connects to the server. If it fails, it will try again in 3 seconds.
Then, a listener thread is spawned which reads from the socket until certain strings are read.
When connected, the user is prompted for the first command. This enters a loop that only breaks
when the user enters "quit". Otherwise, the commands are sent every 2 seconds using sleep(). 
There exists a disconnecter() function which is called to smoothly disconnect the client from
the server.
