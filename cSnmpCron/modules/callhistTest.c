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

#include <util.h>
#include <module.h>

#include <pthread.h>
static pthread_mutex_t snmpmutex_main;

main(int argc, char *argv[])
{

    int res=0, j;
    struct timespec rqtp;

    pthread_mutex_init(&(snmpmutex_main), NULL);   

    rqtp.tv_sec=0;
    rqtp.tv_nsec=400 * 1000;    // 400us

    taskresult_t taskresult;
    memset( taskresult.message, 0, sizeof(taskresult.message));
    taskresult.snmpmutex=&(snmpmutex_main);

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
