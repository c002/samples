/** 
 *
 * load_targets_from_file()
 *	Using the previous BTA SNMP implementation file format
 * 	to load the targets from an external file.
 *	Format is:
 *
 *	    # Comment
 *	    !sourcehost
 *		target1
 *		...
 *	 	targetN
 *
 * 
 * 
 * 
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <netdb.h>

#include <common.h>
#include <job.h>

static char version[] ="
extern char targetfilepath[];

void free_targetlist(char *targetlist[])
{
     int i=0;
     char *t;

     while (targetlist[i++] && i<MAX_TARGETS)
	free(targetlist[i]);
    return;
}

char **load_targets_from_file(char *fname, char *targetlist[])
{
    char **res;
    char *pattern1="^#.*$";			    /* Pattern for comments */
    char *pattern2="^!([[:print:]]+)";	 	    /* Pattern for source host */	
    char *pattern3="^[[:space:]]+([[:print:]]+)$";  /* Pattern for a target */
    // char *s;
    int i=0;
    int BUFSIZE=256;
    char buf[BUFSIZE];
    FILE *f;
    char thishost[BUFSIZE];
    char *p=NULL;
    int found =0;

    if (! (f=fopen(fname,"r"))) 
	return(NULL);

   /* save the path, so we can check mod date and force a refresh */
   strncpy(targetfilepath, fname, strlen(fname));

    gethostname(thishost, BUFSIZE);
    if ((p=strchr(buf,'.'))) *p='\0'; 
    
    while ( fgets(buf, BUFSIZE, f)!=NULL ) {
	strip(buf, '\n');
        res = rematch(pattern1, buf);
        if (res) {
	    continue;
        } else  {
            res = rematch(pattern2, buf);
	    if (res) found=0;
	    if (res && (strcmp(res[1],thishost)==0) )
		found=1;
  	}
        res = rematch(pattern3, buf);
	if (res && found)
	    targetlist[i++]=strdup(res[1]);
    }

    fclose(f);
    targetlist[i]=NULL;

    return(targetlist);
}
