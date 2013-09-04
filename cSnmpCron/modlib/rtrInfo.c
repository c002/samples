/**
 * rtrInfo()
 * 	Get information on the snmp target
 *
 * 
 * $id$
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

static char *MODULE_NAME="rtrInfo.c";

#ifdef TRACE
#define trace(t) va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"trace [%s] [%s]:%d",t?t:"" , __FILE__, __LINE__)
#else
#define trace(t) 
#endif

// #define RETURN(t) va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"Error return [%s]:%d", __FILE__, __LINE__); return(t)
#define RETURN(t) return(t)

#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

static char version[] ="

// int rtrInfo(char *target, snmp_rtrinfo_t *info, taskresult_t *taskresult);
char *makeupstring(unsigned long uptime, char *upstr);

int rtrInfo(u_char *community, char *target, snmp_rtrinfo_t *info, taskresult_t *taskresult)
{
    struct snmp_session session, session_list[MAX_THREADS];
    struct snmp_pdu *response = NULL;
    unsigned long long result = 0;
    char *hardware=NULL;
    char *version=NULL;
    char *uptime;

    trace(target);
    snmp_sess_init(&session_list[taskresult->index]);
    session_list[taskresult->index].peername=target;
    session_list[taskresult->index].version=SNMP_VERSION_2c;
    session_list[taskresult->index].community=community ? community : (u_char *) "public";
    session_list[taskresult->index].community_len = strlen((char *) session_list[taskresult->index].community);

    session_list[taskresult->index].version = SNMP_VERSION_2c;

    trace(target);
    uptime =".1.3.6.1.2.1.1.3.0";
    version=".1.3.6.1.2.1.1.1.0";
    hardware="";

    memset(info, 0, sizeof(snmp_rtrinfo_t));

    trace(target);

    // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"rtrInfo(1) ix=%d, target=%s", taskresult->index, target);

    response = snmp_get(&session_list[taskresult->index], uptime, taskresult, response);

    trace(target);
    if (response && response->variables) {	/* uptime */
//	variable_list *tmp = response->variables;
        result = (unsigned long) *(response->variables->val.integer);
// XXX debug
//        fprintf(stderr,"target=%s, uptime=%llu\n", target, result);
	if (result)
	    info->uptime=result;

    } else { 
	RETURN(-1);
    }
    // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"rtrInfo(2) ix=%d, target=%s, result=%lu", taskresult->index, target, result);


    if (response && response->command == 0)
	fprintf(stderr,"response pdu may already be free\n");

    trace(target);
    snmp_free_pdu(response);
    trace(target);
    response = snmp_get(&session_list[taskresult->index], version, taskresult, response);
    trace(target);
    if (response && response->variables != NULL ) {	/* version */
	strncpy(info->description,(char *) response->variables->val.string, sizeof(info->description)-1);

    } else { 
	RETURN(-1);
    }
    trace(target);	/* crash count: 1) 18may07 00:05 */
    snmp_free_pdu(response);
    trace(target);
    return(0);
}

