#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <sys/time.h>
#include <signal.h>
#include <semaphore.h>
#include "bankingServer.h"
#define PORT 25569
#define MAXUSERS 1024
#define TIMERMS 15000 //ms to repeat the printing

struct sockaddr_in serverAddressInfo; 
struct sockaddr clientAddressInfo;
int sockfd, clilen, readValue, threadCount, createAllowed;
int newsockfd[MAXUSERS];
char buffer[1024];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t tid[MAXUSERS];
sem_t semmy;
int socketNames[MAXUSERS]; //Index is the corresponding thread #
Node * listHead;

//SIGINT Ctrl+C Handler
void handle_sigint( int sig )
{
	printf( "User shutdown detected. Starting soft end.\n" );
	int j;
	for( j = 0; j < threadCount; j++ )
	{
		send( socketNames[j], "Server shutting down.", strlen( "Server shutting down." ), 0 );
		
		//Closing what needs to be closed
		close( socketNames[j] );
		pthread_cancel( tid[j] );
		sem_destroy( &semmy );
		Node * current;
		while( listHead != NULL ) 
		{
			current = listHead;
			listHead = listHead->next;
			free( current );
		}
	}
	exit( 0 );
}

void * clientThread( void * servSocket )
{
	char threadBuffer[1024];
	Node * serviceNode = NULL;
	//Keep listening for commands
	while( 1 )
	{
		//Var setup
		int sokt = *((int *)servSocket);		
	
		//Parsing for no duplicate data
		int buflen;
		readValue = read( sokt, (char*)&buflen, sizeof(buflen) );
		buflen = ntohl( buflen );
		
		//Read some data
		readValue = read( sokt, threadBuffer, buflen );
		
		/*
		CREATE
		1) Check if client is in service
		2) Check if account name already exists
		3) Add a new account struct to linked list
		4) Initial balance = 0, INSERVICE = 0
		Remember to mutex this.
		*/
		char dest[1024];
		memset( dest, '\0', sizeof( dest ) );
		strncpy( dest, threadBuffer, 6 );
		if( strcmp( stringLower( dest ), "create" ) == 0 )
		{
			//Data stuff
			char inName[255];
			threadBuffer[256] = 0;
			strcpy( inName, &threadBuffer[7] );
			
			inName[ strlen(inName) - 1 ] = 0;
			printf( "Creating account for user %d...name is %s\n", sokt, inName );
			
			
			if( serviceNode != NULL )
			{
				send( sokt, "ERROR: Already in session.", strlen( "ERROR: Already in session." ), 0 );
			}
			else
			{
			if( createAllowed == 0 )
				send( sokt, "Server busy. Please try again.", strlen( "Server busy. Please try again." ), 0 );
			else
			{

			Node * dupeChecker = accountFinder( listHead, inName );
			if( dupeChecker != NULL )
				send( sokt, "Account with this name already exists.", strlen( "Account with this name already exists." ), 0 );
			else
			{

			//Make the new struct
			BankAccount accountBuilder;
			strcpy( accountBuilder.name, inName );
			accountBuilder.balance = 0.0;
			accountBuilder.inSession = 0;


			pthread_mutex_lock( &lock );
			createAllowed = 0;
			if( listHead == NULL )
			{
				listHead = (Node *)malloc(sizeof(Node));
				listHead->accountData = accountBuilder;
			}
			else
			{
	
				//Add node
				Node * current = listHead;
				while( current->next != NULL )
				{
					current = current->next;
				}
				
				current->next = malloc( sizeof(Node) );
				current->next->accountData = accountBuilder;
				current->next->next = NULL;
			}	
			createAllowed = 1;
			pthread_mutex_unlock( &lock );

			send( sokt, "Created your account.", strlen( "Created your account." ), 0 );
			}
			}
			}
		}
		
		/*
		SERVE
		1) Check if INSERVICE == 1
		2) Set client's INSERVICE to 1
		*/
		memset( dest, '\0', sizeof( dest ) );
		strncpy( dest, threadBuffer, 5 );
		if( strcmp( stringLower( dest ), "serve" ) == 0 )
		{
			if( serviceNode != NULL )
			{
				send( sokt, "ERROR: Already in session.", strlen( "ERROR: Already in session." ), 0 );
			}
			else
			{
				char inName[255];
				strcpy( inName, &threadBuffer[6] );
				inName[ strlen(inName) - 1 ] = 0;
				
				Node * current = accountFinder( listHead, inName );

				if( current == NULL )
				{
					send( sokt, "ERROR: Name not found.", strlen( "ERROR: Name not found." ), 0 );
				}
				else
				{		
					if( current->accountData.inSession == 0 )
					{
						send( sokt, "Starting session.", strlen( "Starting session." ), 0 );
						current->accountData.inSession = 1;
						serviceNode = current;
					}
					else
					{
						send( sokt, "ERROR: Already in session.", strlen( "ERROR: Already in session." ), 0 );
					}
				}
			}
		}

		/*
		DEPOSIT
		1) Check if INSERVICE == 1
		2) Add input amount to account balance
		*/
		memset( dest, '\0', sizeof( dest ) );
		strncpy( dest, threadBuffer, 7 );
		if( strcmp( stringLower( dest ), "deposit" ) == 0 )
		{
			if( serviceNode == NULL )
			{
				send( sokt, "ERROR: Not currently in session.", strlen( "ERROR: Not currently in session." ), 0 );
			}
			else
			{
				char inVal[255];
				strcpy( inVal, &threadBuffer[8] );
				inVal[ strlen(inVal) - 1 ] = 0;
				//Check if double input
				double amt = atof( inVal );
				

				Node * current = accountFinder( listHead, serviceNode->accountData.name );
			
				current->accountData.balance = current->accountData.balance + amt;
			
				send( sokt, "Deposited money.", strlen( "Deposited money." ), 0 );
					
			}
		}
		
		/*
		WITHDRAW
		1) Check if INSERVICE == 1
		2) Subtract input amount from account balance
		*/
		memset( dest, '\0', sizeof( dest ) );
		strncpy( dest, threadBuffer, 8 );
		if( strcmp( stringLower( dest ), "withdraw" ) == 0 )
		{
			if( serviceNode == NULL )
			{
				send( sokt, "ERROR: Not currently in session.", strlen( "ERROR: Not currently in session." ), 0 );
			}
			else
			{
				char inVal[255];
				strcpy( inVal, &threadBuffer[9] );
				inVal[ strlen(inVal) - 1 ] = 0;
				//Check if double input
				double amt = atof( inVal );
			

				Node * current = accountFinder( listHead, serviceNode->accountData.name );
				
				if( amt > current->accountData.balance )
				{
					send( sokt, "ERROR: Not enough funds!", strlen( "ERROR: Not enough funds!" ), 0 );
				}
				else
				{
					current->accountData.balance = current->accountData.balance - amt;
					send( sokt, "Withdrew money.", strlen( "Withdrew money." ), 0 );
				}
					
			}
		}
		
		/*
		QUERY
		1) Check if INSERVICE == 1
		2) Get account balance of given account name
		3) Send it back!
		*/
		memset( dest, '\0', sizeof( dest ) );
		strncpy( dest, threadBuffer, 5 );
		if( strcmp( stringLower( dest ), "query" ) == 0 )
		{
			if( serviceNode == NULL )
			{
				send( sokt, "ERROR: Not currently in session.", strlen( "ERROR: Not currently in session." ), 0 );
			}
			else
			{
				char message[255];
				sprintf( message, "Current account balance: $%.2f.", serviceNode->accountData.balance );
				send( sokt, message, strlen( message ), 0 );
			}
		}
		
		
		/*
		END
		1) Check if INSERVICE == 1
		2) Set INSERVICE = 0
		*/
		memset( dest, '\0', sizeof( dest ) );
		strncpy( dest, threadBuffer, 3 );
		if( strcmp( stringLower( dest ), "end" ) == 0 )
		{
			if( serviceNode == NULL )
			{
				send( sokt, "ERROR: Not currently in session.", strlen( "ERROR: Not currently in session." ), 0 );
			}
			else
			{
				serviceNode->accountData.inSession = 0;
				serviceNode = NULL;
				send( sokt, "Ended service with current account.", strlen( "Ended service with current account." ), 0 );
			}
			//printf( "User %d ended their current account session.\n", sokt );
		}
		
		//QUIT is below!
		
		//If they exit, quit smoothly
		if( strcmp( threadBuffer, "quit\n" ) == 0 )
		{
			if( serviceNode )
			{
				serviceNode->accountData.inSession = 0;
				serviceNode = NULL;
			}
			printf( "User %d exited. Closing thread.\n", sokt );
			pthread_exit( NULL );
			close( sokt );
		}

		//Comms
		memset( threadBuffer, 0, sizeof( threadBuffer ) );
	}
}

//Runs every 20 seconds
void timer_handler( int signum )
{
	sem_wait( &semmy );
	createAllowed = 0;
	printf( "\n[---------------All Accounts---------------]\n" );
	Node * current = listHead;
	while( current != NULL ) 
	{
		printf( "Name: %s\t\t", current->accountData.name );
		printf( "Balance: $%.2f\t\t", current->accountData.balance );
		if( current->accountData.inSession == 1 )
			printf( "IN SERVICE\n" );
		else
			printf( "\n" );
		current=current->next;
	}
	printf( "[------------------------------------------]\n" );
	createAllowed = 1;
	sem_post( &semmy );
}

int main( int argc, char const *argv[] )
{
	
	//Signal handler and timer setup
	signal( SIGINT, handle_sigint );
	sem_init( &semmy, 0, 1 );
	createAllowed = 1;

	//struct sigaction sa;
	struct itimerval timer;
	signal( SIGALRM, ( void (*)(int) ) timer_handler );

	//Timer setup specifically
	timer.it_value.tv_sec = TIMERMS/1000;
	timer.it_value.tv_usec = (TIMERMS*1000) % 1000000;
	timer.it_interval = timer.it_value;

	setitimer( ITIMER_REAL, &timer, NULL );
	
	//Socket stuff
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	clilen = sizeof( clientAddressInfo );
	bzero((char *)&serverAddressInfo, sizeof( serverAddressInfo ));
	
	//MAKE SURE TO TEST FOR ENOUGH ARGUMENTS!
	serverAddressInfo.sin_port = htons( atoi( argv[1] ) );
	serverAddressInfo.sin_family = AF_INET;
	serverAddressInfo.sin_addr.s_addr = INADDR_ANY;
	
	//Start to bind
	if ( bind( sockfd, (struct sockaddr *)&serverAddressInfo, sizeof( serverAddressInfo ) ) < 0 )
	{
		perror( "Error on binding." );
		exit( EXIT_FAILURE );
	}
	
	//Listen to clients & start their thread
	listen( sockfd, MAXUSERS );
	int i = 0;
	while( 1 )
	{
		newsockfd[i] = accept( sockfd, (struct sockaddr *)&serverAddressInfo, &clilen );
		
		//Thread stuff
		pthread_attr_t attr;
		pthread_attr_init( &attr );
		socketNames[i] = newsockfd[i];
		pthread_create( &tid[i], &attr, clientThread, &newsockfd[i] );
		printf( "User %d connected. PID: %d\n", newsockfd[i], tid[i] );
		i++;
		
		
		//So that we can join them all later
		threadCount++;
		
	}
	
	printf( "[SERVER]: User exited. Shutting down.\n" );
	
	return 0;
}
