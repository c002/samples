/**
 * snmpget()
 * 	Do an snmp get on target
 *
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
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

/* pthread error messages */
#define PML_ERR "pthread_mutex_lock error\n"
#define PMU_ERR "pthread_mutex_unlock error\n"
#define PCW_ERR "pthread_cond_wait error\n"
#define PCB_ERR "pthread_cond_broadcast error\n"

#define PT_MUTEX_LOCK(x) if (pthread_mutex_lock(x) != 0) fprintf(stderr, PML_ERR);
#define PT_MUTEX_UNLOCK(x) if (pthread_mutex_unlock(x) != 0) fprintf(stderr,PMU_ERR);

#include <module.h>

#define MODULE_NAME "snmpget.c"
#ifdef TRACE
#define trace(s) va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"trace [%s] [%s]:%d", s,__FILE__, __LINE__)
#else
#define trace(s)
#endif

#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

static char version[] ="

int dump_session(char *s, struct snmp_session *sessp, struct snmp_pdu *pdu, struct snmp_pdu *response, taskresult_t *taskresult, char *dest, netsnmp_transport *trans);
char *makeupstring(unsigned long uptime, char *upstr);

/* pass response in */
struct snmp_pdu *snmp_get(struct snmp_session *session, char  *objoid, taskresult_t *taskresult, struct snmp_pdu *response)
{

    struct snmp_pdu *pdu = NULL;
    oid 	anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN;
    char storedoid[BUFSIZE];
    int status = 0;
    // struct snmp_pdu *response = NULL;
    char message[1024];
    void *slp = NULL;
    struct snmp_session *sessp = NULL;
    char result_string[BUFSIZE];
    netsnmp_transport *trans;
    char *dest_txt;

    anOID_len = MAX_OID_LEN;

    PT_MUTEX_LOCK(taskresult->snmpmutex);	// This bit needs to lock, or we can get
    sessp = snmp_sess_open(session);		// a session from another thread.
    if (sessp)
        trans  = snmp_sess_transport(sessp);
    PT_MUTEX_UNLOCK(taskresult->snmpmutex);

    if (!sessp || !trans) {
	if (taskresult) {

	    taskresult->status=session->s_snmp_errno;
	    strncpy(taskresult->message,snmp_api_errstring(session->s_snmp_errno), sizeof(taskresult->message)-1);
	} else
	    fprintf(stderr, "sessp null: %s\n", snmp_api_errstring(session->s_snmp_errno));
	return(NULL);
    }

    pdu = snmp_pdu_create(SNMP_MSG_GET);
    if (! pdu) {
	if (taskresult) {
	    taskresult->status=-1;
	    strncpy(taskresult->message,"Internal error, failed to create PDU", sizeof(taskresult->message)-1);
	} else
	    fprintf(stderr, "Failed to create PDU\n");
	snmp_sess_close(sessp);
	return(NULL);
    }

    dest_txt = trans->f_fmtaddr(trans, pdu->transport_data, pdu->transport_data_length);

    read_objid(objoid, anOID, &anOID_len);
    strncpy(storedoid, objoid, sizeof(storedoid)-1);

    snmp_add_null_var(pdu, anOID, anOID_len);
 
    status = snmp_sess_synch_response(sessp, pdu, &response);

    if (taskresult) {
        taskresult->status=status;
        if (status == STAT_DESCRIP_ERROR) {
            sprintf(message, "*** SNMP Error: (%s) Bad descriptor.\n", dest_txt);
	    strncpy(taskresult->message, message, sizeof(taskresult->message)-1);
        } else if (status == STAT_TIMEOUT) {
            sprintf(message, "*** SNMP No response: (%s@%s).\n", dest_txt, storedoid);
	    strncpy(taskresult->message, message, sizeof(taskresult->message)-1);
        } else if (status != STAT_SUCCESS) {
            sprintf(message, "*** SNMP Error: (%s@%s) Unsuccessuful (%d).\n", dest_txt,storedoid, status);
	    strncpy(taskresult->message, message, sizeof(taskresult->message)-1);
        } else if (status == STAT_SUCCESS && response && response->errstat == SNMP_ERR_NOERROR) {
            sprintf(message, "*** SNMP Success (%d): (%s@%s) %s\n", status, dest_txt, storedoid, snmp_errstring(response->errstat));
	    strncpy(taskresult->message, message, sizeof(taskresult->message)-1);
        } else if (status == STAT_SUCCESS) {
            sprintf(message, "*** SNMP Success (%d): (%s@%s) %s\n", status, dest_txt, storedoid);
	    strncpy(taskresult->message, message, sizeof(taskresult->message)-1);
        }
    }

    if (dest_txt) SNMP_FREE(dest_txt);
    snmp_sess_close(sessp);

    if (status == STAT_SUCCESS && response && response->variables != NULL)
        return (response);
    else
	return(NULL);
} 

/**
 * Takes a millisecond value from a routers uptime and format
 * in easy to read days,hours,min etc.
 */
char *makeupstring(unsigned long uptime, char *upstr)
{

    unsigned long leftovers;
    double seconds;
    double days;
    double hours;
    double mins;
    double secs;
    double msecs;

    seconds = uptime / 100;

    hours = modf(seconds / (3600 * 24) , &days);
    hours = hours * 24;

    mins = modf(hours, &hours);
    mins = mins * 60;

    secs = modf(mins, &mins);
    secs = secs * 60;

    msecs = modf(secs, &secs);
    msecs = msecs * 60;
    
    sprintf(upstr, "%lu Days, %.2u:%.2u:%.2u.%.3u", 
	(int)days, (unsigned int) hours, 
	(unsigned int) mins, (int) secs, (int) msecs);

    return(upstr);    

}

/* debug. Dump session contents */
int
dump_session(char *s, struct snmp_session *sessp, struct snmp_pdu *pdu, struct snmp_pdu *response, taskresult_t *taskresult, char *dest, netsnmp_transport *trans)
{
    netsnmp_session *ss;
    ss = snmp_sess_session(sessp);
    if (! ss->localname)
	ss->localname="NA";

    if (dest==NULL)
	dest="null";

    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,
		"SESSIONDUMP[%s] [peername=%s] [dst=%s] [ix=%d] [remport=%d] [localport=%d] [localname=%s] [reqid=%ld] [transid=%ld] [&pdu=x%lu] [&sess=%lu] [&trans=%lu]", s,
		ss->peername, dest,
		taskresult->index,
		ss->remote_port, 
		ss->local_port, 
		ss->localname,
		pdu->reqid,
		pdu->transid,
		pdu, sessp, trans);

    if (response!=NULL) 
        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,
		"SESSIONDUMP[%s] [peername=%s] [dst=%s] [ix=%d] [remport=%d] [localport=%d] [localname=%s] [preqid=%ld] [rreqid=%ld] [ptransid=%ld] [rtransid=%ld]  [&pdu=x%lu] [&resp=x%lu]", s,
		ss->peername, dest,
		taskresult->index,
		ss->remote_port, 
		ss->local_port, 
		ss->localname,
		pdu->reqid, response->reqid,
		pdu->transid, response->transid,
		pdu, response);


	return 0;
}
