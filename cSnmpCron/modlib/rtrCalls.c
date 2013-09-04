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

#include <math.h>
#include <hashtable.h>

#include <util.h>
#include <module.h>

#define MODULE_NAME "rtrCalls.c"

#ifdef TRACE
#define trace() va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"trace [%s]:%d", __FILE__, __LINE__)
#else
#define trace()
#endif

// #define RETURN(t) va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"Error Return [%s]:%d", __FILE__, __LINE__); return(t)
#define RETURN(t) return(t)


#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

static char version[] ="

static unsigned int hashfromkey(void *ky);
static int equalkeys(void *k1, void *k2);
static int hget(struct hashtable *data_hash, int key);
static int hput(struct hashtable *data_hash, int key, int ix);

int fetchcallhist(struct snmp_session *session,snmp_callhist_t *calls, struct hashtable *data_hash, char *field, taskresult_t *taskresult);

/*
 * rtrCalls()
 *	fetches some columns from the CallHistoryTable
 *
 * returns -1 on error , 0 on success.
 */
int rtrCalls(u_char *community, char *target, snmp_callhist_t *calls, taskresult_t *taskresult)
{
    int res, i;
    unsigned int max_index=0;
    struct snmp_session session, session_list[MAX_THREADS];
    int status = 0;
    size_t anOID_len = MAX_OID_LEN;
    oid anOID[MAX_OID_LEN];
    char result_string[BUFSIZE];
    unsigned long long result = 0;
    struct hashtable  *data_hash;

    trace();
    /* netsnmp print/sprint  functions look at these, we just
   	want the data */

    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
		NETSNMP_DS_LIB_QUICK_PRINT,1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
		NETSNMP_DS_LIB_PRINT_BARE_VALUE,1);

    memset(calls, 0, MAX_INTERFACES * sizeof(snmp_callhist_t));

    trace();
    /* This is for an oid table to array index map */
    data_hash = create_hashtable(123, hashfromkey, equalkeys);

    snmp_sess_init(&session_list[taskresult->index]);

    trace();
    /* some of this to go into a cfg file */
    session_list[taskresult->index].peername=target;
    session_list[taskresult->index].version=SNMP_VERSION_2c;
    session_list[taskresult->index].community=community ? community : (u_char *) "public";
    session_list[taskresult->index].community_len =  strlen((char*) session_list[taskresult->index].community);
    session_list[taskresult->index].version = SNMP_VERSION_2c;

    res = fetchcallhist(&session_list[taskresult->index], calls, data_hash, "DestinationHostName", taskresult);
    if (res==0)
        res = fetchcallhist(&session_list[taskresult->index], calls, data_hash, "TransmitBytes", taskresult);
    if (res==0)
        res=fetchcallhist(&session_list[taskresult->index], calls, data_hash, "ReceiveBytes", taskresult);

    hashtable_destroy(data_hash,1);

    return(res);
}

/***
 * fetchcallhist()
 *	Do a bulkget on a single column of a table
 */
int
fetchcallhist(struct snmp_session *session,snmp_callhist_t *calls, struct hashtable *data_hash, char *field, taskresult_t *taskresult)
{
    snmp_data_t *d;
    snmp_data_t *sdata;
    int localindex=0, max_index=0, tmpix;

    h_data *p;
    h_key  key;

    char *ciscoCallHistory= ".1.3.6.1.4.1.9.9.27.1.1.3.1";
    char *InterfaceNumber=".1.3.6.1.4.1.9.9.27.1.1.3.1.7";
    char *DestinationHostName=".1.3.6.1.4.1.9.9.27.1.1.3.1.7";
    char *TransmitBytes=".1.3.6.1.4.1.9.9.27.1.1.3.1.15";
    char *ReceiveBytes=".1.3.6.1.4.1.9.9.27.1.1.3.1.17";

    trace();
    if (strcmp(field, "DestinationHostName")==0)
        sdata = snmp_bulk(session, DestinationHostName, taskresult, sdata);
    else if (strcmp(field, "TransmitBytes")==0)
        sdata = snmp_bulk(session, TransmitBytes, taskresult, sdata);
    else if (strcmp(field, "ReceiveBytes")==0)
        sdata = snmp_bulk(session, ReceiveBytes, taskresult, sdata);

    if (! sdata) RETURN(-1);

    trace();
    for (d = sdata; d; d = d->next) {
	if (!d || !d->buf || !d->index)
	    continue;

	tmpix=hget(data_hash, d->index);
	if ( tmpix<0 )
	    hput(data_hash, d->index, localindex);
	else 
	    localindex=tmpix;

	assert(localindex<MAX_INTERFACES);

	calls[localindex].index=d->index;

	if (strcmp(field, "DestinationHostName")==0) 
	    strncpy(calls[localindex].DestinationHostName, strip(d->buf,'\"'), sizeof(calls[p->aindex].DestinationHostName));
	else if (strcmp(field, "TransmitBytes")==0)
	    calls[localindex].TransmitBytes= atol(strip(d->buf,'\"'));
	else if (strcmp(field, "ReceiveBytes")==0)
	    calls[localindex].ReceiveBytes = atol(strip(d->buf,'\"'));

	localindex++;

	if (localindex > max_index) 
	    max_index=localindex;
    }
    calls[max_index+1].index=-1;	/* sentinal marker for end of data */

    trace();
    free_snmp_data(sdata);

    trace();
    return(0);
}

/**
 * Get array index from hash
 */
static int hget(struct hashtable *data_hash, int key)
{

    int *p;

    if (! data_hash) RETURN(-1);

    if ((p = hashtable_search(data_hash, &key)))
        return(*p);
    else
        RETURN(-1);

}
static int hput(struct hashtable *data_hash, int key, int ix)
{

    int *p, *k , *v;
  
    k = (int *)   malloc(sizeof(int));
    v = (int *)   malloc(sizeof(int));

    *k = key, 
    *v = ix;

    if (! data_hash) RETURN(-1);

    if (! hashtable_insert(data_hash, k, v))
        RETURN(-1);

    return(0); 
}

static unsigned int hashfromkey(void *ky)
{
    int i, hash;
    int *k = (int *) ky;

    for (i=0; i<sizeof(*k); i++)
    {
        hash =+ *k;
        hash += (hash << 10);
        hash ^= (hash >>6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    
    return(hash);
}

static int
equalkeys(void *k1, void *k2)
{
    int *key1 = (int *) k1;
    int *key2 = (int *) k2;
    return (*key1 == *key2);
}

