/**
 * lifted from snmplib/snmp_client.c
 * so that I can put mutex wrappers around the callback. 
 *
 * 
 * 
**/

#ifndef _REENTRANT
#define _REENTRANT
#endif

static char *MODULE_NAME= "xsnmp_sess.c";

#include <errno.h>
#include <pthread.h>

#include <common.h>

#include <net-snmp/net-snmp-config.h>        
#include <net-snmp/net-snmp-includes.h>        

#include <net-snmp/types.h>        
       
#include <net-snmp/library/snmp_api.h>        
#include <net-snmp/library/snmp_client.h>        
#include <net-snmp/library/snmp_secmod.h>        
#include <net-snmp/library/mib.h>        
#include <net-snmp/library/snmp_logging.h>        
#include <net-snmp/library/snmp_assert.h> 

/* pthread error messages */
#define PML_ERR "pthread_mutex_lock error\n"
#define PMU_ERR "pthread_mutex_unlock error\n"
#define PCW_ERR "pthread_cond_wait error\n"
#define PCB_ERR "pthread_cond_broadcast error\n"

#define PT_MUTEX_LOCK(x) if (pthread_mutex_lock(x) != 0) fprintf(stderr, PML_ERR);
#define PT_MUTEX_UNLOCK(x) if (pthread_mutex_unlock(x) != 0) fprintf(stderr,PMU_ERR);

static pthread_mutex_t *snmpmutex_local;

static char *selectlist(fd_set *fdset,  int numfds, char *bitstr);

static int      xsnmp_synch_input(int op, netsnmp_session * session,        
                                 int reqid, netsnmp_pdu *pdu, void *magic);
int
xsnmp_sess_synch_response(void *sessp,
                         netsnmp_pdu *pdu, netsnmp_pdu **response, pthread_mutex_t *snmpmutex);

int
xsnmp_sess_synch_response(void *sessp,
                         netsnmp_pdu *pdu, netsnmp_pdu **response, pthread_mutex_t *snmpmutex)
{
    netsnmp_session *ss;
    struct synch_state lstate, *state;
    snmp_callback   cbsav;
    void           *cbmagsav;
    int             numfds, count;
    fd_set          fdset;
    struct timeval  timeout, *tvp;
    int             block;

    char bitstr[2048]={0};

    snmpmutex_local = snmpmutex;

    ss = snmp_sess_session(sessp);
    memset((void *) &lstate, 0, sizeof(lstate));
    state = &lstate;
    cbsav = ss->callback;
    cbmagsav = ss->callback_magic;
    ss->callback = xsnmp_synch_input;
    ss->callback_magic = (void *) state;

    if ((state->reqid = snmp_sess_send(sessp, pdu)) == 0) {
        snmp_free_pdu(pdu);
        state->status = STAT_ERROR;
    } else
        state->waiting = 1;

    while (state->waiting) {
        numfds = 0;
        FD_ZERO(&fdset);
        block = SNMPBLOCK;
        tvp = &timeout;
        timerclear(tvp);
        snmp_sess_select_info(sessp, &numfds, &fdset, tvp, &block);
        if (block == 1)
            tvp = NULL;         /* block without timeout */

//debug
//        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"XSYNC SEL: %s",
//        	selectlist(&fdset,  numfds, bitstr));

        count = select(numfds, &fdset, 0, 0, tvp);

        if (count > 0) {
    // PT_MUTEX_LOCK(snmpmutex_local);
            snmp_sess_read(sessp, &fdset);
    // PT_MUTEX_UNLOCK(snmpmutex_local);
        } else
            switch (count) {
            case 0:
                snmp_sess_timeout(sessp);
                break;
            case -1:
                if (errno == EINTR) {
                    continue;
                } else {
                    snmp_errno = SNMPERR_GENERR;    /*MTCRITICAL_RESOURCE */
                    /*
                     * CAUTION! if another thread closed the socket(s)
                     * waited on here, the session structure was freed.
                     * It would be nice, but we can't rely on the pointer.
                     * ss->s_snmp_errno = SNMPERR_GENERR;
                     * ss->s_errno = errno;
                     */
                    snmp_set_detail(strerror(errno));
                }
                /*
                 * FALLTHRU 
                 */
            default:
                state->status = STAT_ERROR;
                state->waiting = 0;
            }
    }
    *response = state->pdu;
//debug
    if (state->pdu && state->pdu->variables && 
		(state->pdu->variables->type == ASN_TIMETICKS
		|| state->pdu->variables->type == ASN_COUNTER ) )
          va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,
                "XSYNC [target=%s] [reqid=%ld] [value=%lu] ", ss->peername,  pdu->reqid, (unsigned long) *(state->pdu->variables->val.integer));
// end debug
    ss->callback = cbsav;
    ss->callback_magic = cbmagsav;
    return state->status;
}


static int
xsnmp_synch_input(int op,
                 netsnmp_session * session,
                 int reqid, netsnmp_pdu *pdu, void *magic)
{
    struct synch_state *state = (struct synch_state *) magic;
    int             rpt_type;

    if (reqid != state->reqid && pdu && pdu->command != SNMP_MSG_REPORT) {
        return 0;
    }

    //PT_MUTEX_LOCK(snmpmutex_local);

    state->waiting = 0;

    if (op == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE && pdu) {
        if (pdu->command == SNMP_MSG_REPORT) {
            rpt_type = snmpv3_get_report_type(pdu);
            if (SNMPV3_IGNORE_UNAUTH_REPORTS ||
                rpt_type == SNMPERR_NOT_IN_TIME_WINDOW) {
                state->waiting = 1;
            }
            state->pdu = NULL;
            state->status = STAT_ERROR;
            session->s_snmp_errno = rpt_type;
            SET_SNMP_ERROR(rpt_type);
        } else if (pdu->command == SNMP_MSG_RESPONSE) {
            /*
             * clone the pdu to return to snmp_synch_response 
             */
            state->pdu = snmp_clone_pdu(pdu);
            state->status = STAT_SUCCESS;
            session->s_snmp_errno = SNMPERR_SUCCESS;
        }
        else {
            char msg_buf[50];
            state->status = STAT_ERROR;
            session->s_snmp_errno = SNMPERR_PROTOCOL;
            SET_SNMP_ERROR(SNMPERR_PROTOCOL);
            snprintf(msg_buf, sizeof(msg_buf), "Expected RESPONSE-PDU but got %s-PDU",
                     snmp_pdu_type(pdu->command));
            snmp_set_detail(msg_buf);

	    PT_MUTEX_UNLOCK(snmpmutex_local);

            return 0;
        }
    } else if (op == NETSNMP_CALLBACK_OP_TIMED_OUT) {
        state->pdu = NULL;
        state->status = STAT_TIMEOUT;
        session->s_snmp_errno = SNMPERR_TIMEOUT;
        SET_SNMP_ERROR(SNMPERR_TIMEOUT);
    } else if (op == NETSNMP_CALLBACK_OP_DISCONNECT) {
        state->pdu = NULL;
        state->status = STAT_ERROR;
        session->s_snmp_errno = SNMPERR_ABORT;
        SET_SNMP_ERROR(SNMPERR_ABORT);
    }

    //PT_MUTEX_UNLOCK(snmpmutex_local);
    return 1;
}

char *selectlist(fd_set *fdset,  int numfds, char *bitstr)
{
     int bit, i;
     char bs[1];

     if (! fdset || numfds<=0 || numfds>2047)
        return "NA";    

     for (i=0; i<=numfds; i++) {      
        bit = FD_ISSET(i, fdset);
        sprintf(bs, "%d", bit);
        strcat(bitstr, bs);
    }
    return(bitstr);
}
