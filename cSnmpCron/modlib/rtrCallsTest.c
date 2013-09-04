/**
 * TODO:
 * rtrCalls()
 *	Get all ifInoctets and ifOutOctets for routers
 *	by bulk.  Always give results as 64bit.
 *	All interface >= 20Mb, use HC
 *
 * 
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <util.h>
#include <module.h>

#include <pthread.h>

#define STAT_DESCRIP_ERROR 99

static char version[] ="

static pthread_mutex_t snmpmutex_main;

main(int argc, char *argv[])
{

    int res=0, i=0;
    unsigned max_index;
    taskresult_t taskresult;
    snmp_callhist_t calls[MAX_INTERFACES];

    pthread_mutex_init(&(snmpmutex_main), NULL);
    taskresult.snmpmutex=&snmpmutex_main;

    while (1) {
       if (argc>2)
            res = rtrCalls(argv[2], argv[1], calls, &taskresult);
        else {
            fprintf(stderr, "Usage: %s target community\n", argv[0]);
            exit(0);
        }

        if (res!=0) {
	    fprintf(stderr, "Error\n");
	    fprintf(stderr, "error: (%d) %s\n", taskresult.status, taskresult.message);
        } else {

	    fprintf(stderr, "%3s %20s %12s %12s\n", 
		    "Ix","DestHost","RcvdBytes","TxBytes");
	    for (i=0; (calls[i].index>=0 && i<MAX_INTERFACES); i++) {
                fprintf(stderr, "%3d %20s %12u %12u\n", 
                    calls[i].index,
                    calls[i].DestinationHostName,
                    calls[i].ReceiveBytes,
                    calls[i].TransmitBytes);
  
	    }
	}
    }

}
