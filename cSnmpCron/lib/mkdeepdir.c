/**
 * 
 * Create directories as needed for a given path
 *
 * $Source: /usr/local/cvsroot/cSnmpCron/lib/mkdeepdir.c,v $
 * $Id: mkdeepdir.c,v 1.1.1.1 2008/05/29 05:21:37 harry Exp $
 */
#ifndef _REENTRANT
#define _REENTRANT
#endif

#define DEBUG 0

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <util.h>

int mkdeepdir(const char *path, mode_t mode)
{

    char *last;
    int val;

    val = mkdeepdir_r(path, mode, &last);

    return(val);

}

int mkdeepdir_r(const char *path, mode_t mode, char **last)
{

    char *token;
    char *temp;
    FILE *f;
    char *dpath=NULL;
    struct stat statbuf;
    int res=0, sz;
    char *fpath="/tmp/mkdeepdir_r.debug";

    temp=strdup(path);

#ifdef DEBUG
    if (f=fopen(fpath,"a")) {
        lockf(fileno(f), F_LOCK, 0);
  	fprintf(f,"mkdeepdir_start: [%s] &temp=%x\n" , path, temp);
	lockf(fileno(f), F_ULOCK, 0);
	fclose(f);
    }
#endif

    token=strtok_r(temp,"/", last);
    while (token) 
    {
	sz= (dpath ? strlen(dpath):0) + strlen(token)+3;
	if (dpath)
	    dpath = (char *) realloc(dpath, sz);
	else {
	    dpath = (char *) calloc(sz, sizeof(char));
	    // if (strncmp(path, "~/", 2)==0)
	    if (strncmp(path, "/", 1)==0)
		strcat(dpath,"/");	// Absolute path
	}

	if (!dpath) {
		free(temp);
		return(errno);
	}

	strcat(dpath,token);
	strcat(dpath,"/");

	if ( stat(dpath, &statbuf)<0) {
	    if ((res=mkdir(dpath,mode))<0) {
		if (errno!=EEXIST) {	// for multithreaded , so ignore.
		    free(temp);
		    free(dpath);
		    return(errno);
		} 
	    }
	}
        token=strtok_r(NULL,"/", last);
    }
    free(temp);
    free(dpath);

#ifdef DEBUG
    if (f=fopen(fpath,"a")) {
        lockf(fileno(f), F_LOCK, 0);
  	fprintf(f,"    mkdeepdir_end: &temp=%x\n", temp );
	lockf(fileno(f), F_ULOCK, 0);
	fclose(f);
    }
#endif
    return(0);

}

#ifdef MAIN
main()
{
    int res;
    char *dir="/var/log/snmp/1160644800";

    res=mkdeepdir(dir, 0755);
    fprintf(stderr,"%d\n", res);
    if (res != 0 ) {
	printf("error=%d, %s (%s)\n", res, strerror(res), dir);
	perror("mkdir");
    }
}
#endif
