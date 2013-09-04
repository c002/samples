/*
 * $Source: /usr/local/cvsroot/cSnmpCron/lib/exception.c,v $
 * $Id: exception.c,v 1.1.1.1 2008/05/29 05:21:37 harry Exp $
 */

#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>


// #include "exception.h"
#include "common.h"

#define MODULE_NAME "exception"
#define MAX_FORMAT_STRBUF 2048

static const char* spool_dir;
static char  logfile[PATH_MAX];

/* The min priority for reporting */
static int      filter_priority;
static bool dostderr;

static const char* priority_names[] =
{
    "FATAL",
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG",
};

bool exception_init(int priority, char *fname)
{

    filter_priority=priority;
    if (! fname) {
	dostderr=TRUE;
        strcpy(logfile,"/dev/null");
    } else {
	dostderr=FALSE;
        strcpy(logfile, fname);
    }

    return TRUE;

}

void
va_exception(const char*        mod_name,
             int                priority,
             const char*        format,
             ...)
{
    char        strbuf[MAX_FORMAT_STRBUF];
    va_list ap;
    va_start(ap, format);
    
#ifdef HAVE_VSNPRINTF
    vsnprintf(strbuf, MAX_FORMAT_STRBUF, format, ap);
#else
    vsprintf(strbuf, format, ap);
#endif
    exception(mod_name, priority, strbuf);
    
    va_end(ap);
}

const void rolllog()
{

    char *newf, buf[8];
    time_t t;
    struct tm *pt, pt_r;
    int day;
    FILE *f;

    t=time(NULL);
    pt= localtime_r(&t, &pt_r);
    strftime(&buf[0], sizeof(buf), "%j",pt);
    day = atoi(buf) % 7; 

    newf=malloc(strlen(logfile) + (5 * (sizeof(char))) );
    sprintf(newf,"%s.%d", logfile, day);

    if ( (f=fopen(newf,"r"))!=NULL) {
	fclose(f);
	remove(newf);
    }
	
    rename(logfile, newf);
    free(newf);
}

void
exception(const char* mod_name,
          int priority,
          const char* details)
{
    FILE* log;
    char buf[1000];
    ulong current_time = time(NULL);

    /* Ignore low priority messages */
    if (filter_priority < priority)
        return;

    /* Figure out the name of a new log file if we need one */
    
    if ((log = fopen(logfile, "a")) != NULL)
    {
	if (fileno(log) != fileno(stdout))
            lockf(fileno(log), F_LOCK, 0); 

        /* Write a line to the log file */
        fprintf(log, "%s %s %s: %s\n",
                format_timestamp_r(current_time, buf),
                exception_priority_name(priority),
                mod_name,
                details);
    	if (fileno(log) != fileno(stdout)) {
       	    lockf(fileno(log), F_ULOCK, 0); 
    	}

        fclose(log);
    }
    else
    {
        fprintf(stderr, 
                "Could not open exception log file %s: %s\n",
                logfile,
                strerror(errno));
    }

    /* Print to stderr too */
    if (dostderr==TRUE)
        fprintf(stderr, "%s %s %s: %s\n",
            format_timestamp_r(current_time, buf),
            exception_priority_name(priority),
            mod_name,
            details);
}

const char* format_timestamp_r(const time_t ts, char *buf)
{
    struct tm   *t;
    struct tm pt_r;

    t =  localtime_r(&ts, &pt_r); /* Structure copy */

    strftime(buf, 1000, "%d-%b-%Y %H:%M:%S", t);
    
    return buf;
}

const char* format_timestamp(const time_t ts)
{
    static char buf[1000];
    struct tm pt_r;
    // struct tm   t = *gmtime(&ts); /* Structure copy */
    struct tm   *t;

    t =  localtime_r(&ts, &pt_r); /* Structure copy */

    strftime(buf, 1000, "%d-%b-%Y %H:%M:%S", t);
    
    return buf;
}

const char* exception_priority_name(int priority)
{
    if (   priority >= 0 
        && priority < (sizeof(priority_names) / sizeof(char*)))
    {
        return (priority_names[priority]);
    }
    else
    {
        return "*UNKNOWN PRIORITY*";
    }
}

