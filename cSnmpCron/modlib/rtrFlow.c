/**
 * rtrInfo()
 * 	Get information on the snmp target
 *
 */

#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <module.h>

#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

// int rtrInfo(char *target, snmp_rtrinfo_t *info, taskresult_t *taskresult);
char *makeupstring(unsigned long uptime, char *upstr);

#ifdef MAIN
main(int argc, char *argv[])
{

    int res=0, i=0;
    snmp_rtrinfo_t info;
    taskresult_t taskresult;
    char upstr[1024];
    struct timespec rqtp;

    rqtp.tv_sec=0;
    rqtp.tv_nsec=400 * 1000;    // 400us

    //for (i=0;i<100;i++) {
    while (1) {
        taskresult.message[0]=NULL;

        if (argc>1)
            res = rtrFlows("ccanet", argv[1], &info, &taskresult);
        else
            res = rtrFlows("ccanet","localhost", &info, &taskresult);

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
#endif

int rtrFlows(char *community, char *target, snmp_rtrinfo_t *info, taskresult_t *taskresult)
{
    struct snmp_session session;
    struct snmp_pdu *response = NULL;
    unsigned long long result = 0;
    char *hardware=NULL;
    char *version=NULL;
    char *uptime;

    snmp_sess_init(&session);
    session.peername=target;
    session.version=SNMP_VERSION_2c;
    session.community=community ? community : "public";
    session.community_len = strlen(session.community);

    session.version = SNMP_VERSION_2c;

    flows =".1.3.6.1.2.1.1.3.0";

    response = snmp_get(&session, flows, taskresult, response);

    if (response) {	/* uptime */
        result = (unsigned long) *(response->variables->val.integer);
        // fprintf(stderr,"uptime=%llu", result);
	info->uptime=result;

    } else { 
	return(-1);
    }

    if (response->command == 0)
	fprintf(stderr,"response pdu may already be free\n");

    snmp_free_pdu(response);

    response = snmp_get(&session, version, taskresult, response);
    if (response) {	/* version */
	strncpy(info->description,response->variables->val.string, sizeof(info->description)-1);

    } else { 
	return(-1);
    }

    if (response->command == 0)
	fprintf(stderr,"2 response pdu may already be free\n");
    snmp_free_pdu(response);

    return(0);
}

