/**
 */

#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <module.h>

#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

int store_results(config_t *config, joblist_t *job, taskresult_t *taskresult, char *target, snmp_rtrinfo_t *info, snmp_octets_t *octets);

#ifdef MAIN
main(int argc, char *argv[])
{

    int res=0;
    struct timespec rqtp;

    rqtp.tv_sec=0;
    rqtp.tv_nsec=400 * 1000;    // 400us

    taskresult_t taskresult;

    while(1) {

        if (argc>1)
            res = flows_run(argv[1],NULL, NULL, &taskresult);
        else
            res = flows_run("localhost",NULL, NULL, &taskresult);

        if (res!=0) {
	    fprintf(stderr, "Error (%d) %s\n");
            fprintf(stderr, "status=%d, %s\n", taskresult.status, taskresult.message);
	}
	nanosleep(&rqtp, NULL);
    } 

}
#endif

int flows_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult)
{
    int res;
    snmp_rtrinfo_t info;
    char result_string[BUFSIZE];
    char *community=NULL;
    snmp_octets_t octets[MAX_INTERFACES];

    memset(octets, 0, sizeof(snmp_octets_t) * MAX_INTERFACES);

    if (config && config->community)
	community=config->community;
    else 
	community="ccanet";

    res = rtrInfo(community, target, &info, taskresult);
    if (res!=0) {
	// fprintf(stderr, "1 ifaces=%d\n", res);
	return(res);
    } else
        res=store_results(config, job, taskresult, target, &info, octets);
 
    return(res);
}


int store_results(config_t *config, joblist_t *job, taskresult_t *taskresult, char *target, snmp_rtrinfo_t *info, snmp_octets_t *octets)
{

    FILE *f;
    char fname[1024];
    char dir[1024];
    char fpath[1024];
    char message[1024];
    int res;
    time_t timestamp=0;
    struct stat statbuf;
    unsigned long long result;

    if (config && job && config->datadir && job->prefix) {
       
        sprintf(dir,"%s/%d", config->datadir, job->timestamp);
        sprintf(fname,"%s.%s.log",job->prefix, target);
        sprintf(fpath,"%s/%s",dir, fname);

        // fprintf(stderr, " -- [outfile=%s] --\n", fpath);
	timestamp=job->timestamp;

        if ( (res = mkdeepdir_r(dir, 0755, &last)) <0) {
	    fprintf(stderr,"mkdeepdir() != 0\n");
	    sprintf(message,"mkdir() %s: [%s]", strerror(errno), dir);
	    if (taskresult) {
	        taskresult->status=errno;
	        //taskresult->message=strdup(message);
	        strncpy(taskresult->message,message, sizeof(taskresult->message)-1);
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
	        //taskresult->message=strdup(message);
	        strncpy(taskresult->message,message, sizeof(taskresult->message)-1);
	    } else {
		fprintf(stderr,"Error opening file\n");
	    }
	    return(-1);
	}

    } else {
	taskresult->status=0;
	strncpy(taskresult->message,"Using stdout as output", sizeof(taskresult->message)-1);

	f=stdout;	
    }

    if (fileno(f) != fileno(stdout))
        lockf(fileno(f), F_LOCK, 0); 

# 1.1 ::: hay-cor10.connect.com.au ::: 1158365040 ::: 3593993745 ::: 3632713863 ::: 34535 ::: 3656484169

    "%s ::: %s ::: %d ::: %d ::: %s ::: %d ::: %d\n"
    fprintf(f, "1.1 ::: %s ::: %lu ::: %lu\n",

version, target, timestamp, 
                 TransmitFlows, TransmitPackets,
                 DroppedPackets, uptime)


		target,
		timestamp,
		info->uptime);

    if (fileno(f) != fileno(stdout)) {
        lockf(fileno(f), F_ULOCK, 0); 
	fclose(f);
    }
 
    return(0);
}
