#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include "bankingClient.h"
#define PORT 25567
#define BREAKOUT "Server shutting down."

struct sockaddr_in serverAddressInfo;
int readVal, sockfd;
struct hostent *serverIPAddress;
char userMsg[255];
char buffer2[255];
char buffer[1024];


//Easy disconnect function why not
void disconnecter()
{
	printf( "[CLIENT]: Disconnecting from server.\n" );
	
	close( sockfd );
	
	exit( 0 );
}

/*
Constantly listens for input.
If the input is anything that should shut down the client, goes to disconnecter
*/
void * listenThread()
{

	while( ( strcmp( buffer, BREAKOUT ) != 0 ) && ( strcmp( stringLower( buffer2 ), "quit" ) != 0 ) )
	{
		memset( buffer, '\0', sizeof( buffer )  );
		readVal = read( sockfd, buffer, 1024 );
		printf( "\n[SERVER]: %s\n", buffer );
	}
	disconnecter();
}

int main( int argc, char const *argv[] )
{
	int sendThis = 0;

	//CLIENTSIDE CONNECTION SETUP
	serverIPAddress = gethostbyname( argv[1] );

	sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	
	bzero( (char *)&serverAddressInfo, sizeof( serverAddressInfo ) );
	serverAddressInfo.sin_family = AF_INET;
	serverAddressInfo.sin_port = htons( atoi( argv[2] ) );
	bcopy( (char *)serverIPAddress->h_addr, (char *)&serverAddressInfo.sin_addr.s_addr, serverIPAddress->h_length );
	
	//Retry connection every 3 seconds
	int connected = 0;
	printf( "Attempting to connect to server...\n" );
	while( connected == 0 )
	{
		if( connect( sockfd, (struct sockaddr *)&serverAddressInfo, sizeof( serverAddressInfo ) ) < 0 )
		{
			printf( "Failed to connect. Retrying in 3 seconds.\n" );
			sleep(3);
			printf( "Retrying connection...\n" );
		}
		else
		{
			connected = 1;
			printf( "Connection successful.\n" );
		}
	}
	
	//LISTENER THREAD SETUP
	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_create( &tid, &attr, listenThread, NULL );
	
	//USER INPUT "THREAD"
	while( 1 )
	{
		sendThis = 0;
		printf( "\nEnter a command: " );
		fgets( userMsg, 255, stdin );
		
	
		//Parsing for no duplicate data
		int datalen = strlen( userMsg );
		int tmp = htonl( datalen );
		
		/*
		If statements to check for proper input before sending.
		*/
		memset( buffer2, '\0', sizeof( buffer2 ) );
		strncpy( buffer2, userMsg, 4 );
		if( strcmp( stringLower( buffer2 ), "quit" ) == 0 )
		{
			send( sockfd, (char *)&tmp, sizeof( tmp ), 0 );
			send( sockfd, stringLower( userMsg ), strlen( userMsg ), 0 );
			disconnecter();
			break;
		}
		
		//Check "create"
		memset( buffer2, '\0', sizeof( buffer2 ) );
		strncpy( buffer2, userMsg, 6 );
		if( strcmp( stringLower( buffer2 ), "create" ) == 0 )
		{
			char outName[255];
			strcpy( outName, &userMsg[7] );
			//check that there is input
			if( outName[0] == '\0' || outName[1] == '\0' )
			{
				printf( "\nERROR: Invalid name.\n" );
				continue;
			}
			sendThis = 1;
		}
		
		//Check "serve"
		memset( buffer2, '\0', sizeof( buffer2 ) );
		strncpy( buffer2, userMsg, 5 );
		if( strcmp( stringLower( buffer2 ), "serve" ) == 0 )
		{
			char outName[255];
			strcpy( outName, &userMsg[6] );
			//check that there is input
			if( outName[0] == '\0' || outName[1] == '\0' )
			{
				printf( "\nERROR: Invalid name.\n" );
				continue;
			}
			sendThis = 1;
		}
		if( strcmp( stringLower( buffer2 ), "query" ) == 0 )
			sendThis = 1;
		
		//Check "deposit"
		memset( buffer2, '\0', sizeof( buffer2 ) );
		strncpy( buffer2, userMsg, 7 );
		if( strcmp( stringLower( buffer2 ), "deposit" ) == 0 )
		{
			char inVal[255];
			strcpy( inVal, &userMsg[8] );
			inVal[ strlen(inVal) - 1 ] = 0;
			//Check if double input
			double amt = atof( inVal );
			
			//Check if can be a double and isn't negative
			if( amt == 0.0 || inVal[0] == '-' )
			{
				printf( "\nERROR: Invalid amount entered.\n" );
				continue;
			}
			sendThis = 1;
		}
		
		//Check "withdraw"
		memset( buffer2, '\0', sizeof( buffer2 ) );
		strncpy( buffer2, userMsg, 8 );
		if( strcmp( stringLower( buffer2 ), "withdraw" ) == 0 )
		{
			char inVal[255];
			strcpy( inVal, &userMsg[9] );
			inVal[ strlen(inVal) - 1 ] = 0;
			//Check if double input
			double amt = atof( inVal );
			
			//Check if can be a double and isn't negative
			if( amt == 0.0 || inVal[0] == '-' )
			{
				printf( "\nERROR: Invalid amount entered.\n" );
				continue;
			}
			sendThis = 1;
		}
			
		memset( buffer2, '\0', sizeof( buffer2 ) );
		strncpy( buffer2, userMsg, 3 );
		if( strcmp( stringLower( buffer2 ), "end" ) == 0 )
			sendThis = 1;
		
		if( sendThis )
		{
			send( sockfd, (char *)&tmp, sizeof( tmp ), 0 );
			send( sockfd, userMsg, strlen( userMsg ), 0 );
		} else {
			printf( "Invalid command. Commands: create, serve, withdraw, deposit, query, end, quit" );
			continue;
		}
	
		sleep(2);		

		
	}
	
	pthread_join( tid, NULL );
	
	disconnecter();
	
	return 0;
}
