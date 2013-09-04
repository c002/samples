/**
 * rtrInfo()
 * 	Get information on the snmp target
 *
 * 
 * $id$
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <pthread.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <module.h>

static char version[] ="

static pthread_mutex_t snmpmutex_main;

main(int argc, char *argv[])
{

    int res=0, i=0;
    snmp_rtrinfo_t info;
    taskresult_t taskresult;
    char upstr[1024];
    struct timespec rqtp;

    rqtp.tv_sec=0;
    rqtp.tv_nsec=400 * 1000;    // 400us

    pthread_mutex_init(&(snmpmutex_main), NULL);
    taskresult.snmpmutex=&(snmpmutex_main);

    //for (i=0;i<100;i++) {
    if (1) {
        memset(taskresult.message,0,sizeof(taskresult.message));

       if (argc>2)
            res = rtrInfo(argv[2], argv[1], &info, &taskresult);
        else {
            fprintf(stderr, "Usage: %s target community\n", argv[0]);
            exit(0);
        }

        if (res!=0) {
	    fprintf(stderr, "Error\n");
        } else {
	    makeupstring(info.uptime, upstr);
            fprintf(stderr, "  Uptime=%lu (%s)\n", info.uptime, upstr);
            fprintf(stderr, "  Descrip=%s\n", info.description);
	}
	nanosleep(&rqtp, NULL);
    }
    exit(0);

}
