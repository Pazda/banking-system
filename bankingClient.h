#ifndef BANKINGCLIENT
#define BANKINGCLIENT

//string.lower() but for a dumb language
char *stringLower(char *str)
{
    unsigned char *mystr = (unsigned char *)str;

    while (*mystr) {
        *mystr = tolower(*mystr);
        mystr++;
    }
    return str;
}



#endif
