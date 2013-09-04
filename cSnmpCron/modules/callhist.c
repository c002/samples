/**
 * callhist.c
 *	Collects stuff from the Cisco callhist MIB
 * 
 * 
 * 
 */

#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <module.h>

#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

static int store_results(config_t *config, joblist_t *job, taskresult_t *taskresult, char *target, snmp_rtrinfo_t *info, snmp_callhist_t *calls);

#ifdef MAIN
main(int argc, char *argv[])
{

    int res=0, j;
    struct timespec rqtp;

    rqtp.tv_sec=0;
    rqtp.tv_nsec=400 * 1000;    // 400us

    taskresult_t taskresult;
    taskresult.message[0]=NULL;

    // for (j=0; j<100; j++) {
    while (1) {
        if (argc>1)
            res = callhist_run(argv[1],NULL, NULL, &taskresult);
        else
            res = callhist_run("localhost",NULL, NULL, &taskresult);

        if (res!=0) {
            fprintf(stderr, "Error\n");
            fprintf(stderr, "status=%d, %s\n", taskresult.status, taskresult.message);
        } 
        nanosleep(&rqtp, NULL);
    }
}
#endif

int callhist_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult)
{
    int res;
    snmp_rtrinfo_t info;
    char result_string[BUFSIZE];
    u_char *community=NULL;
    char *ciscoCallHistory=NULL;
    snmp_callhist_t calls[MAX_INTERFACES];

    if (config && config->community)
	community=config->community;
    else 
	community=(u_char *) "public";

    res = rtrInfo(community, target, &info, taskresult);
    if (res!=0) {
	// fprintf(stderr, "1 ifaces=%d\n", res);
	return(res);
    }

    res = rtrCalls(community, target, calls, taskresult);

    if (res==0) {
        res=store_results(config, job, taskresult, target, &info, calls);
	if (res!=0) 
	    return(res);
	else
       	    return(0);
    } 

    return(res);
}


static int store_results(config_t *config, joblist_t *job, taskresult_t *taskresult, char *target, snmp_rtrinfo_t *info, snmp_callhist_t *calls)
{

    FILE *f;
    char fname[1024];
    char dir[1024];
    char fpath[1024];
    char message[1024];
    char *last;
    int res;
    time_t timestamp=0;
    struct stat statbuf;
    unsigned long long result;

    if (config && job && config->datadir && job->prefix) {
       
        sprintf(dir,"%s/%d", config->datadir, job->timestamp);
        sprintf(fname,"%s.%s.log",job->prefix, target);
        sprintf(fpath,"%s/%s",dir, fname);

        //fprintf(stderr, " ======= -- [outfile=%s] --\n", fpath);
	timestamp=job->timestamp;

        if ( (res = mkdeepdir_r(dir, 0755, &last)) <0) {
	    sprintf(message,"mkdir() %s: [%s]", strerror(errno), dir);
	    if (taskresult) {
	        taskresult->status=errno;
	        strncpy(taskresult->message,strdup(message),sizeof(taskresult->message)-1);
	    } else {
                fprintf(stderr, " create dir: %s\n", dir);
                fprintf(stderr, " failed to stat dir: %s (%d)\n", dir, errno);
	    }
	    return(-1);		
 	}

        if (! (f=fopen(fpath,"a"))) {
	    sprintf(message,"fopen() %s: %s", strerror(errno), dir);
	    if (taskresult) {
	        taskresult->status=errno;
	        strncpy(taskresult->message,strdup(message),sizeof(taskresult->message)-1);
	    } else {
		fprintf(stderr,"Error opening file\n");
	    }
	    return(-1);
	}
    } else {
	timestamp=time(NULL);
	taskresult->status=0;
        strncpy(taskresult->message,"Using stdout as output",sizeof(taskresult->message)-1);

	f=stdout;	
    }

    if (fileno(f) != fileno(stdout))
        lockf(fileno(f), F_LOCK, 0); 

    int i;
    char *sep = config->separator;
    for (i=0; (calls[i].index>=0 && i<MAX_INTERFACES); i++) {

// # 1.1 ::: syd-cor3.connect.com.au ::: 1161097500 ::: 3946941618 ::: 3844459561.943 ::: gmf-gw ::: 6006468 ::: 11282534
            fprintf(f, "%s%s%s%s%lu%s%lu%s%u%s%s%s%u%s%u\n", 
		FMT_VERSION,
		sep,
		target,
		sep,
		timestamp,
		sep,
		info->uptime,
		sep,
                calls[i].index,
		sep,
                calls[i].DestinationHostName,
		sep,
                calls[i].ReceiveBytes,
		sep,
                calls[i].TransmitBytes);
     }

    if (fileno(f) != fileno(stdout)) {
        lockf(fileno(f), F_ULOCK, 0); 
	fclose(f);
    }
 
    return(0);
}
