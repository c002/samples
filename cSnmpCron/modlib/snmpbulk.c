/**
 * snmpbulk()
 *	An lib routing to do a bulk snmp get
 *
 * 
 * $id*
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

#define MODULE_NAME "snmpbulk.c"
#ifdef TRACE
#define trace(s) va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"trace [%s] [%s]:%d", s,__FILE__, __LINE__)
#else
#define trace(s)
#endif

#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

static char version[] ="

snmp_data_t *snmp_bulk(struct snmp_session *session, char  *objoid, taskresult_t *taskresult, snmp_data_t *head)
{
    int i, check, running=1;
    struct snmp_pdu *pdu = NULL;
    // char storedoid[BUFSIZE];
    int status = 0;
    struct snmp_pdu *response = NULL;
    snmp_data_t *return_data = NULL; //, *head=NULL;
    struct variable_list *vars = NULL;
    char message[1024];
    //struct snmp_session *sessp = NULL;
    netsnmp_transport *trans;
    void *sessp = NULL;
    char result_string[BUFSIZE];
    oid  name[MAX_OID_LEN];
    oid             root[MAX_OID_LEN];
    size_t rootlen = MAX_OID_LEN;
    size_t name_len = rootlen;
    unsigned long sz;
    int buf_len = 128;
    char buf[buf_len];
    char *dest_txt=NULL;

    head=NULL;
    
    PT_MUTEX_LOCK(taskresult->snmpmutex);
    sessp = snmp_sess_open(session);		// needs to be locked or we 
    if (sessp) 					// can get a session form another thread.
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

    read_objid(objoid, root, &rootlen);
    memmove(name, root, rootlen * sizeof(oid));
    name_len = rootlen;	
    
    while (running) {

        pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
        pdu->non_repeaters = 0;
        pdu->max_repetitions = 10;    /* fill the packet */

        snmp_add_null_var(pdu, name, name_len);

        status = snmp_sess_synch_response(sessp, pdu, &response);

  	// To get the target, which is not avail in the sessp struct and we don't
	// want to reference the thread unsafe session struct anymore
        dest_txt = trans->f_fmtaddr(trans, pdu->transport_data, pdu->transport_data_length);

        if (status != STAT_SUCCESS || !response) {

            if (taskresult) {
                taskresult->status=status;

                if (status == STAT_DESCRIP_ERROR) {
                     sprintf(message, "*** SNMP Error: (%s) Bad descriptor.\n", dest_txt);
		     strncpy(taskresult->message, message, sizeof(taskresult->message)-1);
                } else if (status == STAT_TIMEOUT) {
                    sprintf(message, "*** SNMP No response: (%s@%s).\n", dest_txt, objoid);
		     strncpy(taskresult->message, message, sizeof(taskresult->message)-1);
                } else if (status != STAT_SUCCESS) {
                    sprintf(message, "*** SNMP Error: (%s@%s) Unsuccessuful (%d).\n", dest_txt,objoid, status);
		     strncpy(taskresult->message, message, sizeof(taskresult->message)-1);
                } else if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
                    sprintf(message, "*** SNMP Success (%d): (%s@%s) %s\n", status, dest_txt, objoid, snmp_errstring(response->errstat));
		     strncpy(taskresult->message, message, sizeof(taskresult->message)-1);
                }
            }

	    if (dest_txt) SNMP_FREE(dest_txt);
	    if (response) snmp_free_pdu(response);
    	    if (sessp != NULL) snmp_sess_close(sessp);
	    return(NULL);
       }
       trace(dest_txt);

       if (response->errstat == SNMP_ERR_NOSUCHNAME) {
	        if (taskresult) {
		    taskresult->status=SNMP_ERR_NOSUCHNAME;
		     strncpy(taskresult->message,"End of MIB", sizeof(taskresult->message)-1);
	        } else
                    fprintf(stderr,"End of MIB\n");
       }	// Line:125 hay-cor9 errcount=1, removed dup block
       if (response->variables->type == SNMP_NOSUCHINSTANCE)
	        if (taskresult) {
		    taskresult->status=SNMP_NOSUCHINSTANCE;
		    strncpy(taskresult->message,"SNMP_NOSUCHINSTANCE", sizeof(taskresult->message)-1);
	        } else
                    fprintf(stderr,"SNMP_NOSUCHINSTANCE\n");

	for (vars = response->variables; (vars && running); vars = vars->next_variable)
	{

            if ((vars->type == SNMP_ENDOFMIBVIEW) ||
                (vars->type == SNMP_NOSUCHOBJECT) ||
                (vars->type == SNMP_NOSUCHINSTANCE)) {
		running=0;
		continue;
	    }

	    if (snmp_oid_compare(name, name_len, vars->name, vars->name_length) >= 0) {
		running=0;
		continue;
	    }

	    if ((vars->name_length < rootlen)
                        || (memcmp(root, vars->name, rootlen * sizeof(oid))
                            != 0)) {
		// not part of this subtree
		    /*
	        if (taskresult) {
		    taskresult->status=0;
	  	    taskresult->message=strdup("End of Subtree");
	        } else
		    fprintf(stderr,"End of SubTree\n");
		    */
		running=0;
		continue;
	    }

	    // print_variable(vars->name, vars->name_length, vars);

	    if (vars->next_variable == NULL) {
                memmove(name, vars->name,
                     vars->name_length * sizeof(oid));
                     name_len= vars->name_length;
            }

	    if (! head) {
	        head = malloc(sizeof(snmp_data_t));
		return_data=head;
	    } else {
	        return_data->next = malloc(sizeof(snmp_data_t));
		return_data= return_data->next;
	    }

	    return_data->name = malloc(vars->name_length * sizeof(oid));
	    memcpy(return_data->name , vars->name, vars->name_length * sizeof(oid));
	    return_data->name_length = vars->name_length;
	    return_data->type = vars->type;
	    return_data->index = vars->name[vars->name_length-1];
	
	    snprint_variable(buf, buf_len, vars->name, vars->name_length, vars);
	    strcpy(return_data->buf, buf);

	    return_data->next= NULL;

	}
	if (response)
            snmp_free_pdu(response);

	if (taskresult) {
	    if (! head) {
	        taskresult->status=-1;
	    	sprintf(message,"*** SNMP Error: (%s@%s) No such object", dest_txt, objoid);
	    } else {
	        taskresult->status=0;
	    	sprintf(message,"*** SNMP Success: (%s@%s)", dest_txt, objoid);
	    }
	    strncpy(taskresult->message, message, sizeof(taskresult->message));
	}

        if (dest_txt) SNMP_FREE(dest_txt);

    } /* while running */
    if (sessp != NULL)
           snmp_sess_close(sessp);
    return(head);
} 

void free_snmp_data(snmp_data_t *data)
{
    if (data) {
	if (data->next)
	    free_snmp_data(data->next);
	if (data->name) 
		free(data->name);
	data->next=NULL;
        free(data);
    }
}
