/**
 * interfaces
 *	Collects 32 or 64 bit ifoctets data
 * 
 * 
 $ 
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

#include <pthread.h>

#define MODULE_NAME "interfaces.c"

#ifdef TRACE
#define trace() va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"trace [%s]:%d", __FILE__, __LINE__)
#else
#define trace() 
#endif

// #define RETURN(t) va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"Error return [%s]:%d", __FILE__, __LINE__); return(t)
#define RETURN(t) return(t)

#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

static int store_results(config_t *config, joblist_t *job, taskresult_t *taskresult, char *target, snmp_rtrinfo_t *info, snmp_octets_t *octets);

#ifdef MAIN
main(int argc, char *argv[])
{

    int res=0, j;
    struct timespec rqtp;

    rqtp.tv_sec=0;
    rqtp.tv_nsec=400 * 1000;    // 400us

    taskresult_t taskresult;
    taskresult.message[0]=NULL;

    for (j=0; j<100; j++) {
        if (argc>1)
            res = interfaces_run(argv[1],NULL, NULL, &taskresult);
        else
            res = interfaces_run("localhost",NULL, NULL, &taskresult);

        if (res!=0) {
            fprintf(stderr, "Error\n");
            fprintf(stderr, "status=%d, %s\n", taskresult.status, taskresult.message);
        } 
        nanosleep(&rqtp, NULL);
    }
}
#endif

int interfaces_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult)
{
    int res;
    snmp_rtrinfo_t info;
    char result_string[BUFSIZE];
    u_char *community=NULL;
    snmp_octets_t octets[MAX_INTERFACES];

    // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Interfaces run start for %s", target);

    if (config && config->community)
	community=config->community;
    else 
	community="ccanet";

    trace();
    // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Interfaces run (1) for %s", target);
    res = rtrInfo(community, target, &info, taskresult);
    trace();
    if (res!=0) {
        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Interfaces run (2) for %s", target);
	// fprintf(stderr, "1 ifaces=%d\n", res);
	RETURN(res);
    }

    // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Interfaces run (2.5) for %s (%lu)", target, info.uptime);

    trace();
    res = rtrOctets(community, target, octets, taskresult);
    // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Interfaces run (3) for %s", target);
    trace();
    if (res==0) {
	trace();
        res=store_results(config, job, taskresult, target, &info, octets);
	trace();

	// fprintf(stderr, "2 ifaces=%d\n", res);
        // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Interfaces run done (res=%d) for %s", res, target);
	return(res);
    } 
    //fprintf(stderr, "3 ifaces=%d\n", res);
    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Interfaces run done (non-0) for %s", target);
    RETURN(res);
}


static int store_results(config_t *config, joblist_t *job, taskresult_t *taskresult, char *target, snmp_rtrinfo_t *info, snmp_octets_t *octets)
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

    // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Store results start %s", target);
    if (config && job && config->datadir && job->prefix) {
        trace();   
        sprintf(dir,"%s/%d", config->datadir, job->timestamp);
        sprintf(fname,"%s.%s.log",job->prefix, target);
        sprintf(fpath,"%s/%s",dir, fname);

        // fprintf(stderr, " -- [outfile=%s] --\n", fpath);
	timestamp=job->timestamp;
	
	trace();

        if ( (res = mkdeepdir_r(dir, 0755, &last)) <0) {
	    sprintf(message,"mkdir() %s: [%s]", strerror(errno), dir);
	    if (taskresult) {
	        taskresult->status=errno;
	        strncpy(taskresult->message,message,sizeof(taskresult->message)-1);
	    } else {
                fprintf(stderr, " create dir: %s\n", dir);
                fprintf(stderr, " failed to stat dir: %s (%d)\n", dir, errno);
	    }
	    RETURN(-1);		
 	}

	trace();

        if (! (f=fopen(fpath,"a"))) {
	    sprintf(message,"fopen() %s: %s", strerror(errno), dir);
	    if (taskresult) {
	        taskresult->status=errno;
	        strncpy(taskresult->message,message,sizeof(taskresult->message)-1);
	    } else {
		fprintf(stderr,"Error opening file\n");
	    }
	    RETURN(-1);
	}

    } else {
	taskresult->status=0;
        strncpy(taskresult->message,"Using stdout as output",sizeof(taskresult->message)-1);

	f=stdout;	
    }

    if (fileno(f) != fileno(stdout))
        lockf(fileno(f), F_LOCK, 0); 

    int i;
    char *sep = config->separator;

    trace();

    for (i=0; (octets[i].index>=0 && i<MAX_INTERFACES); i++) {
        if (octets[i].operstatus==1)	/* Only output interfaces that are up */ 
            fprintf(f, "1.1%s%s%s%lu%s%lu%s%s %d%s%lu%s%ld%s%llu%s%llu\n", 
		sep,
		target,
		sep,
		timestamp,
		sep,
		info->uptime,
		sep,
                octets[i].interface, octets[i].bits,
		sep,
                octets[i].bandwidth,
		sep,
                octets[i].index,
		sep,
                octets[i].ifinoctets,
		sep,
                octets[i].ifoutoctets);
     }

    if (fileno(f) != fileno(stdout)) {
        lockf(fileno(f), F_ULOCK, 0); 
	fclose(f);
    }
    // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Store results done %s", target);
 
    return(0);
}
