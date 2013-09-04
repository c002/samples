/* retools
 *
 * Regex wrapper tool
 *
 * $Source: /usr/local/cvsroot/cSnmpCron/lib/retools.c,v $
 * $Id: retools.c,v 1.1.1.1 2008/05/29 05:21:37 harry Exp $
 */

// (?P<mtr>[a-z]+-[a-z]+\d+)\s+:=\s*(?P<flow>[^#\\]*).*?(?P<cont>\\*)$

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#define NMATCH 1024

char **rematch(char *pattern, char *string);

int reerror=0;
char *reerrstr="";

char **rematch(char *pattern, char *string)
{
    regex_t *re;
    regmatch_t subs[NMATCH];

    static char *str[NMATCH], **head=NULL;
    char s[1024], *p=NULL;
    int size;
    int res=0;
    int eflags=0,i;

    if (!pattern || !string) return(NULL);

    re = (regex_t *) malloc(sizeof (regex_t));

    reerror = regcomp(re, pattern, REG_EXTENDED);
    if (reerror!=0) {
	switch(reerror) {

	    case 0 : reerrstr="Success";
			      break;
	    case REG_NOMATCH : reerrstr="No match found";
			      break;
	    case REG_ECOLLATE : reerrstr="Not implmented";
			      break;
	    case REG_EESCAPE : reerrstr="Trailing backslash";
			      break;
	    case REG_ESUBREG : reerrstr="Invalid back reference";
			      break;
	    case REG_EBRACK : reerrstr="Unmatched left bracket";
			      break;
	    case REG_EPAREN : reerrstr="Parenthesis imbalance";
			      break;
	    case REG_EBRACE : reerrstr="Unmatched {";
			      break;
	    case REG_BADBR : reerrstr="Invalid contents of {}";
			      break;
	    case REG_ERANGE : reerrstr="Invalid range end";
			      break;
	    case REG_ESPACE : reerrstr="Ran out of memory";
			      break;
	    case REG_BADRPT : reerrstr="No preceding re for repetition op";
			      break;
	    default: reerrstr="";
		     break;
	}
        regfree(re);
        free(re);
  	return(NULL);
    }

    // fprintf(stderr, "0: [%d] = [%s]\n", reerror, reerrstr);

    res = regexec(re, string, NMATCH, subs, eflags);

    regfree(re);
    free(re);

    if (res == 0 ) {
	/* Matches */

	head=str;
	int j=0;
	for (i=0; i<NMATCH;i++) {
	    //strncpy(s, (string + subs[i].rm_so), (subs[i].rm_eo - subs[i].rm_so)+1);
	    memset(s,0,sizeof(s));
	    strncpy(s, (string + subs[i].rm_so), (subs[i].rm_eo - subs[i].rm_so));
	    // printf("  so=%d eo=%d, size=%d\n", subs[i].rm_so, subs[i].rm_eo, (subs[i].rm_eo - subs[i].rm_so));	
	    if (!s || strlen(s)==0)
		break;
	    size = (subs[i].rm_eo - subs[i].rm_so);
	    // printf("[%d] s=[%s], sz=%d (%s)\n", i, s, size, string);
	    str[j] = (char *) calloc(strlen(s)+1, 1);
	    strncpy(str[j], s, strlen(s)+1);  
	    *(str[j]+size)='\0';

	    // printf("  sub=%s (off=%d size=%d)\n", str[j], subs[i].rm_so, size);	
	    j++;
	    str[j]=NULL;
	}
        return(head);
    } else
	return(NULL);
}

#ifdef MAIN
main()
{
    char **res;
    char *pattern="^\\([0-9,*]\\+\\)[[:space:]]\\+\\([0-9*]\\+\\)$";
    char *string="2,3,4,5,6 *";
    //char *pattern=".*\\(fred\\).*\\(was\\).*";
    //char *string="yes fred was here";
    char *p;

    res = rematch(pattern, string);

    if (!res) {
        fprintf(stderr, "[%d] = [%s]\n", reerror, reerrstr);
        exit(-1);
    } 

   
    for(p=*res; *res!=NULL; *res++) {
        printf("p=%s\n",p); 
    }   

}
#endif
