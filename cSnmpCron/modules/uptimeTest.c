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

#include <pthread.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <module.h>

#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

static pthread_mutex_t snmpmutex_main;

int store_results(config_t *config, joblist_t *job, taskresult_t *taskresult, char *target, snmp_rtrinfo_t *info, snmp_octets_t *octets);

main(int argc, char *argv[])
{

    int res=0;
    struct timespec rqtp;

    pthread_mutex_init(&(snmpmutex_main), NULL);

    rqtp.tv_sec=0;
    rqtp.tv_nsec=400 * 1000;    // 400us

    taskresult_t taskresult;
    taskresult.snmpmutex=&(snmpmutex_main);

    while(1) {

        if (argc>1)
            res = uptime_run(argv[1],NULL, NULL, &taskresult);
        else
            res = uptime_run("localhost",NULL, NULL, &taskresult);

        if (res!=0) {
	    fprintf(stderr, "Error (%d) %s\n");
            fprintf(stderr, "status=%d, %s\n", taskresult.status, taskresult.message);
	}
	nanosleep(&rqtp, NULL);
    } 

}
