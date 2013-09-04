/**
 * rtrOctetsTest
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <pthread.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <common.h>
#include <util.h>
#include <module.h>

static pthread_mutex_t snmpmutex_main;

static char version[] ="

main(int argc, char *argv[])
{

    int res=0, i=0;
    unsigned max_index;
    snmp_rtrinfo_t info;
    taskresult_t taskresult;
    snmp_octets_t octets[MAX_INTERFACES];
    struct timespec rqtp;

    pthread_mutex_init(&(snmpmutex_main), NULL);
    taskresult.snmpmutex=&(snmpmutex_main);

    rqtp.tv_sec=0;
    rqtp.tv_nsec=400 * 1000;    // 400us

    if (1) {
	memset(taskresult.message,0,sizeof(taskresult.message));
       if (argc>2)
            res = rtrOctets(argv[2], argv[1], octets, &taskresult);
        else {
            fprintf(stderr, "Usage: %s target community\n", argv[0]);
            exit(0);
        }

/* ---
        if (argc>1)
            res = rtrOctets((u_char *) "ccanet", argv[1], octets, &taskresult);
        else
            res = rtrOctets((u_char *) "ccanet", "localhost", octets, &taskresult);
--- */
        if (res!=0) {
	    fprintf(stderr, "Error\n");

	    fprintf(stderr, "error: (%d) %s\n", taskresult.status, taskresult.message);
        } else {
	    fprintf(stderr, "%30s (%4s) %20s %20s\n", "Interface","ix","InOctets","OutOctets");
	    for (i=0; (octets[i].index>=0 && i<MAX_INTERFACES); i++) {
	        if (octets[i].index && (octets[i].operstatus==1) && (octets[i].ifinoctets || octets[i].ifoutoctets)) 
	        //if (octets[i].index)
		    fprintf(stderr, "%30s (%4u) %20llu %20llu (%d bit) %12lu\n", 
			octets[i].interface,
			octets[i].index,
			octets[i].ifinoctets,
			octets[i].ifoutoctets,
			octets[i].bits,
			octets[i].bandwidth);
	    }
	}
        nanosleep(&rqtp, NULL);
    }

}
