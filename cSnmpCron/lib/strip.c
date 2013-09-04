/***
 * Remove all occurances of char c in s
 *
 * $Source: /usr/local/cvsroot/cSnmpCron/lib/strip.c,v $
 * $Id: strip.c,v 1.1.1.1 2008/05/29 05:21:37 harry Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *strip(char *s, char c)
{
    char *tstr;
    int i=0, j=0;
    
    if (!s) return(NULL);
 
    tstr= (char *) calloc(strlen(s)+1,1);

    for (i=0; i<=strlen(s);i++)
        if (s[i]!=c) 
	    tstr[j++]=s[i];

    strncpy(s, tstr, j);
    free(tstr);
    return(s);
}
