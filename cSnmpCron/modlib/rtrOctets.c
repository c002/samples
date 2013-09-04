/**
 * rtrOctets()
 *	Get all ifInoctets and ifOutOctets for routers
 *	by bulk.  Always give results as 64bit.
 *	All interfaces >= 20Mb, use HC
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

#include <common.h>
#include <util.h>
#include <module.h>

#define MODULE_NAME "rtrOctets.c"

#ifdef TRACE
#define trace(t) va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"trace [%s] [%s]:%d", t,__FILE__, __LINE__)
#else
#define trace(t)
#endif

// #define RETURN(t) va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"Error return [%s]:%d", __FILE__, __LINE__); return(t)
#define RETURN(t)  return(t)

#define BUFSIZE 1024
#define STAT_DESCRIP_ERROR 99

static char version[] ="

static unsigned int hashfromkey(void *ky);
static int equalkeys(void *k1, void *k2);
static int hget(struct hashtable *data_hash, int key);
static int hput(struct hashtable *data_hash, int key, int ix);

/*
 * Provides Interface info in the snmp_octet_t struct
 * returns -1 on error , 0 on success.
 */
int rtrOctets(u_char *community, char *target, snmp_octets_t *octets, taskresult_t *taskresult)
{
    void *sessp = NULL;
    int res, i;
    unsigned int max_index=0;
    struct snmp_session session, session_list[MAX_THREADS];
    snmp_data_t *sdata=NULL, *d;
    char result_string[BUFSIZE];
    char message[1024];
    char *hcinocts=NULL, *bandwidth=NULL,*operstatus=NULL ;
    char *hcoutocts=NULL, *ifinoctets, *ifoutoctets;
    char *highspeed, *interfaces;
    int ix, tmpix, error=0;
    struct hashtable *data_hash;

    /* netsnmp print/sprint  functions look at these, we just
   	want the data */

    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
		NETSNMP_DS_LIB_QUICK_PRINT,1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
		NETSNMP_DS_LIB_PRINT_BARE_VALUE,1);

    memset(octets, 0, MAX_INTERFACES * sizeof(snmp_octets_t));

    trace(target);

    snmp_sess_init(&session_list[taskresult->index]);

    /* some of this to go into a cfg file */
    session_list[taskresult->index].peername=target;
    session_list[taskresult->index].version=SNMP_VERSION_2c;
    session_list[taskresult->index].community=community ? community : (u_char *) "public";
    session_list[taskresult->index].community_len = strlen((char *) session_list[taskresult->index].community);
    session_list[taskresult->index].version = SNMP_VERSION_2c;

    bandwidth = ".1.3.6.1.2.1.2.2.1.5";
    hcinocts =".1.3.6.1.2.1.31.1.1.1.6";
    hcoutocts =".1.3.6.1.2.1.31.1.1.1.10";
    highspeed=".1.3.6.1.2.1.31.1.1.1.15";
    operstatus=".1.3.6.1.2.1.2.2.1.8";
    interfaces=".1.3.6.1.2.1.2.2.1.2";
    ifinoctets=".1.3.6.1.2.1.2.2.1.10";
    ifoutoctets=".1.3.6.1.2.1.2.2.1.16";

    memset(octets, 0, sizeof(octets));

    trace(target);
    data_hash = create_hashtable(123, hashfromkey, equalkeys);

    trace(target);
    sdata = snmp_bulk(&session_list[taskresult->index], interfaces, taskresult, sdata);
    ix=0;
    for (d = sdata; d; d = d->next) {
	if (IGNORE_VIRTUAL) {
		if (strncmp(strip(d->buf, '\"'), VIRTUAL_IF, strlen(VIRTUAL_IF))==0)
		    continue;
	}

	if ((tmpix=hget(data_hash, d->index))<0)
	    hput(data_hash, d->index, ix);
	else
	    ix=tmpix;
	
	if ( ix >= MAX_INTERFACES) {
	    error=1;
	    if (taskresult) {
                taskresult->status=-1;
                strncpy(taskresult->message,"MAX_INTERFACES exceeded", strlen("MAX_INTERFACES exceeded")+1);
	        break;
	    }
	}

	octets[ix].index=d->index;
	strncpy(octets[ix].interface, strip(d->buf, '\"'), sizeof(octets[ix].interface));

	/* For physical interfaces >20M, we use 64-bit counters , the IfSpeed 
 	   provides the interface configured bandwidth, not the physical bandwidth. 
	   Have seen 32bit for a "bandwidth 10Mbps" cfg interface on a Giga, giving bogus 32bit counters. 
        */

	if (strncmp(octets[ix].interface, FAST_IF, strlen(FAST_IF))==0)
		octets[ix].bits=64;
	else if (strncmp(octets[ix].interface, GIG_IF, strlen(GIG_IF))==0)
		octets[ix].bits=64;

	/* va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO,"iface=%s, bits=%d\n", octets[ix].interface,  octets[ix].bits); */

	ix++;

	if (ix > max_index) 
	    max_index=ix;
    }

    trace(target);

    free_snmp_data(sdata);
    trace(target);

   if (error)
	RETURN(-1);

/*** This maynot be reliable or its an snmp bug or a thread reentrant issue, 
     IfSpeed is also used  to check for highspeed -- 7mar07 hr  
***/
    sdata = snmp_bulk(&session_list[taskresult->index], highspeed, taskresult, sdata);
    ix=0;
    for (d = sdata; d; d = d->next) {
	if ((tmpix=hget(data_hash, d->index))<0)
	    // hput(data_hash, d->index, ix);
	    continue;		// ignored interface
	else
	    ix=tmpix;
	
	assert(ix<MAX_INTERFACES);

	if (atoi(d->buf) > 20) 
	    octets[ix].bits=64;
	else if (octets[ix].bits!=64)
	    octets[ix].bits=32;
	ix++;
	if (ix > max_index) 
	    max_index=ix;
    }

    trace(target);

    free_snmp_data(sdata);
    sdata = snmp_bulk(&session_list[taskresult->index], operstatus, taskresult, sdata);
    ix=0;
    for (d = sdata; d; d = d->next) {
	if ((tmpix=hget(data_hash, d->index))<0)
	    // hput(data_hash, d->index, ix);
	    continue;	/* ignored interface */
	else
	    ix=tmpix;

	assert(ix<MAX_INTERFACES);

	octets[ix].operstatus = atoi(d->buf);

	ix++;
	if (ix > max_index) 
	    max_index=ix;
    }
    trace(target);

    free_snmp_data(sdata);
    sdata = snmp_bulk(&session_list[taskresult->index], bandwidth, taskresult, sdata);
    ix=0;
    for (d = sdata; d; d = d->next) {
	if ((tmpix=hget(data_hash, d->index))<0)
	    //hput(data_hash, d->index, ix);
	    continue;	/* ignored interface */
	else
	    ix=tmpix;
	octets[ix].bandwidth = atol(d->buf);

	if (octets[ix].bits==32 && octets[ix].bandwidth > 20000000) 
	    octets[ix].bits=64;

	assert(ix<MAX_INTERFACES);

	ix++;
	if (ix > max_index) 
	    max_index=ix;
    }
    trace(target);

    free_snmp_data(sdata);
    sdata = snmp_bulk(&session_list[taskresult->index], ifinoctets, taskresult, sdata);
    for (d = sdata; d; d = d->next) {
	    if ((tmpix=hget(data_hash, d->index))<0)
		// hput(data_hash, d->index, ix);
	        continue;	/* ignored interface */
	    else
		ix=tmpix;

	assert(ix<MAX_INTERFACES);
	if ( octets[ix].bits!=64) {
	    octets[ix].ifinoctets = atoll(d->buf);
	    octets[ix].bits=32;
	}
	ix++;
	if (ix > max_index) 
	    max_index=ix;
    }
    trace(target);
    
    free_snmp_data(sdata);
    sdata = snmp_bulk(&session_list[taskresult->index], ifoutoctets, taskresult, sdata);
    ix=0;
    for (d = sdata; d; d = d->next) {
	    if ((tmpix=hget(data_hash, d->index))<0)
		// hput(data_hash, d->index, ix);
	        continue;	/* ignored interface */
	    else
		ix=tmpix;

	assert(ix<MAX_INTERFACES);

	if ( octets[ix].bits!=64) {
	    octets[ix].ifoutoctets = atoll(d->buf);
	    octets[ix].bits=32;
	}
    	ix++;
	if (ix > max_index) 
	    max_index=ix;
    }

    trace(target);
    free_snmp_data(sdata);
    sdata = snmp_bulk(&session_list[taskresult->index], hcoutocts, taskresult, sdata);
    ix=0;
    for (d = sdata; d; d = d->next) {
	    if ((tmpix=hget(data_hash, d->index))<0)
		// hput(data_hash, d->index, ix);
	        continue;	/* ignored interface */
	    else
		ix=tmpix;

	assert(ix<MAX_INTERFACES);

	if ( octets[ix].bits==64) {
	    octets[ix].ifoutoctets = atoll(d->buf);
	}
	ix++;
	if (ix > max_index) 
	    max_index=ix;
    }
    
    trace(target);
    free_snmp_data(sdata);
    sdata = snmp_bulk(&session_list[taskresult->index], hcinocts, taskresult, sdata);
    ix=0;
    for (d = sdata; d; d = d->next) {
	    if ((tmpix=hget(data_hash, d->index))<0)
		// hput(data_hash, d->index, ix);
	        continue;	/* ignored interface */
	    else
		ix=tmpix;
	assert(ix<MAX_INTERFACES);
	if ( octets[ix].bits==64) {
	    octets[ix].ifinoctets = atoll(d->buf);
	}
	ix++;
	if (ix > max_index) 
	    max_index=ix;
    }

    trace(target);
    octets[max_index].index=-1;	/* sentinal marker for end of data */

    hashtable_destroy(data_hash,1);
 
    free_snmp_data(sdata);
    trace(target);
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

