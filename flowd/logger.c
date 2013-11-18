/***
 *
 * A generic logging utility
 * 
 * $Id: logger.c,v 1.1.1.1 2004/05/29 09:06:42 harry Exp $
 * $Author: harry $
 *
 ***/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

void init_logger(char *logfile);
void logger(uint32_t level, const char *format, ...);

extern int loglevel;
int timestamp=1;
FILE *logf=NULL;

void init_logger(char *logfile)
{
    if (! (logf = fopen(logfile, "a+"))) {
	fprintf(stderr,"Failed to open logfile: %s\n",logfile);
	exit(-1);
    }
    
    return;
}

void logger(uint32_t level, const char *format, ...)
{
    
    char*       timestr;
    time_t      t;
    
    if(logf == 0) {
        return;
    }
    
    /*
    ** filter unwanted levels.
    */
    if(level <= loglevel) {
        va_list ap;
        va_start(ap,format);
      
        /*
        ** prefix with timestamp, if required.
        */
        if(timestamp) {
            t = time((time_t*) NULL);
            timestr = ctime(&t);
            if(timestr == NULL) 
                return ;
            timestr[24] = '\0';
            fprintf(logf, "%s: ", timestr);
        }
                    
        vfprintf(logf, format, ap);
        fflush(logf);

        va_end(ap);
    }
}

