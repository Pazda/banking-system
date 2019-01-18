#ifndef BANKINGSERVER
#define BANKINGSERVER

//string.lower() but for an annoying language
char * stringLower(char *str)
{
    unsigned char *mystr = (unsigned char *)str;

    while (*mystr) {
        *mystr = tolower(*mystr);
        mystr++;
    }
    return str;
}

struct BankAccount_;

typedef struct BankAccount_
{

	char name[255];
	double balance;
	int inSession;

} BankAccount;

typedef struct bigNode
{
	struct bigNode * next;
	BankAccount accountData;  
} Node;
  
//Finds account node given. Current should be listHead usually
Node * accountFinder( Node * current, char * inName  )
{
	//current should be list head
	int found = 0;
	while( current != NULL )
        {
              if( strcmp( current->accountData.name, inName ) == 0 )
              {
                 found = 1;
          	  break;
    	      }
       	      current = current->next; //Add error message if not found
        }
        //printf( "found: %d\n", found );

        if( found == 0 )
        {
             return NULL;
        }
	else

	{
		return current;
	}
}

#endif
