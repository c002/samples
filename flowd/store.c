/***
 * Basic raw netflow grabber 
 *
 * Store the flows in a hash and dump to file when flush_interval
 * is up.
 *
 * $Id: store.c,v 1.1.1.1 2004/05/29 09:06:42 harry Exp $
 * $Source: /usr/local/cvsroot/netflow/netflow/flowd/store.c,v $
 ***/

#include <sys/types.h>
#include <limits.h>
#include <symbol.hash.h>	/* libpa */
#include <bgp_api.h>

#include "sock.h"
#include "store.h"

int open_trace_file(void);
int open_debug_file(void); 
void handle_flow(flowdetails *flow);
int close_trace_file(void);
int close_debug_file(void);
void netflow_debug(flowdetails *flow);
int add_flow();
int init_hash();
boolean_t stats_write_entry(const void* k, void* d, void* parameters);

extern char options[];
extern int bgp_communities;
extern symbol_hash_t*   ip_hash;

FILE *trace_f, *debug_f;

/***
 * Incoming netflow data
 ***/
void 
handle_flow(flowdetails *flow)
{

/* Store into hash */
    add_flow(flow);

    if (debug_flows)
        netflow_debug(flow);

}

int close_trace_file(void) 
{
    
    if(trace_f) {
        fclose(trace_f);
    }
    
    trace_f = NULL;
    
    return TRUE;
}
int close_debug_file(void) 
{
    
    if(debug_f) {
        fclose(debug_f);
    }
    
    debug_f = NULL;
    
    return TRUE;
}

int open_debug_file(void) 
{
  
    FILE *f;
    header head;
    int i;
 
    close_debug_file();
 
    if ( debug_count >= max_debug)
            debug_count=0;
    
    sprintf(debug_filename, "%s/debug.%d",
            debug_dir, debug_count++);
    
    if (! (debug_f = fopen(debug_filename, "w")) ) {
                fprintf(stderr,"%s\n",strerror(errno));
                return FALSE;
    } 

    head.sig1=SIG1;
    head.sig2=SIG2;
    head.sig3=SIG3;
    head.version=VERSION;
    for (i=0;i<4;i++)
        head.options[i]=options[i];

    fwrite(&head, sizeof(head), 1, debug_f);

    return TRUE;
}

int open_trace_file(void) 
{
  
    FILE *f;
    header head;
    int i;
 
    close_trace_file();
 
/* See if we want throw the trace away */

    sprintf(notrace_file, "%s/NOTRACE",trace_dir);
    if ( (f=fopen(notrace_file,"r")) ) {
        fclose(f);

        if (! (trace_f=fopen("/dev/null","w")) ) {
                return FALSE;
        }
    } else {
        if ( trace_count >= max_trace)
            trace_count=0;
    
        sprintf(trace_filename, "%s/trace.%d",
            trace_dir, trace_count++);
    
        if (! (trace_f = fopen(trace_filename, "w")) ) {
                fprintf(stderr,"store: %s\n",strerror(errno));
                return FALSE;
        }


	head.sig1=SIG1;
	head.sig2=SIG2;
	head.sig3=SIG3;
	head.version=VERSION;
	for (i=0;i<4;i++)
	    head.options[i]=options[i];

	fwrite(&head, sizeof(head), 1, trace_f);
    } 

    return TRUE;
}

void netflow_debug(flowdetails *flow)
{
    tracedetails trace;

     
    if(debug_f == NULL) {
        open_debug_file();
    }
    
    if(debug_f) {
        trace.timestamp=flow->timestamp;
        trace.srcIP=flow->srcIP;
        trace.dstIP=flow->dstIP;
	trace.srcIDB=flow->srcIDB;
	trace.dstIDB=flow->dstIDB;
        trace.srcport=flow->srcport;
        trace.dstport=flow->dstport;
        trace.protocol=flow->protocol;
        trace.bytes_from=flow->bytes;
        trace.packets_from=flow->packets;
        trace.bytes_to=0;
        trace.packets_to=0;
        trace.srcAS=flow->srcAS;
        trace.dstAS=flow->dstAS;
	trace.bi=0;
	trace.flows=1;
        trace.communities_len=0;
        trace.community1=0;
        trace.community2=0;
        trace.community3=0;
        trace.community4=0;

 	if (bgp_communities==TRUE)
            fwrite(&trace, sizeof(tracedetails), 1,debug_f);
	else
            fwrite(&trace, 46, 1,debug_f);
    }
}

/* initilize hash */
int init_hash()
{
    logger(3,"Initialise hash\n");
    ip_hash = symbol_hash_create(sizeof(ip_key), sizeof(ip_data));
    if (! ip_hash) 
	return -1;

    return 1;
}

/* Add a flow to the hash */
int add_flow(flowdetails *flow)
{ 
    ip_key      key, key2;
    ip_data     *p, data;
    int         i;
    struct      bgp_route_info route;
    struct bgp_attr attr;
    int 	res;

  /* Lookup key in hash */

    key.router  = flow->router;
    key.srcIP   = flow->srcIP;
    key.dstIP   = flow->dstIP;
    key.srcport = flow->srcport;
    key.dstport = flow->dstport;
    key.proto   = flow->protocol;

    if (!ip_hash)
	if (init_hash()<0) {
	     fprintf(stderr,"ip_hash is NULL\n");
	     exit(-1);
        }

/* BGP community lookup */
    if (bgp_communities==TRUE) {
        route.prefix=flow->srcIP;
        route.length=32;
        route.attr= &attr;
        res = bgp_lookup_route(&route);
    }

    if ((p = symbol_hash_lookup(ip_hash, &key)) == NULL) {
        key.router = flow->router;
        key.srcIP  = flow->dstIP; 
        key.dstIP  = flow->srcIP; 
        key.dstport= flow->srcport;
        key.srcport= flow->dstport;
        key.proto  = flow->protocol;

        if ((p = symbol_hash_lookup(ip_hash, &key)) == NULL) {
	    /* new flow */
            key.router  = flow->router;
            key.srcIP   = flow->srcIP;
            key.dstIP   = flow->dstIP;
            key.srcport = flow->srcport;
            key.dstport = flow->dstport;
            key.proto   = flow->protocol;

            memset(&data, 0, sizeof(data));

 	    data.firsttime= flow->timestamp;
	    data.bytes_from= flow->bytes;
	    data.packets_from= flow->packets;
	    data.srcIDB=flow->srcIDB;
	    data.dstIDB=flow->dstIDB;
	    data.srcAS=flow->srcAS;
	    data.dstAS=flow->dstAS;
	    data.flows=1;
	    data.bi=0;

            if ((bgp_communities==TRUE) && res && route.attr->communities_len>0) {
		data.communities_len = route.attr->communities_len;
		data.community1=route.attr->communities[0];
                if (route.attr->communities_len>1)
		    data.community2=route.attr->communities[1];
                if (route.attr->communities_len>2)
		    data.community3=route.attr->communities[2];
                if (route.attr->communities_len>3)
		    data.community4=route.attr->communities[3];
	    }
            symbol_hash_insert(ip_hash, &key, &data);

	} else {	/* reverse flow found  */
	    memcpy(&data,p,sizeof(data));
	    data.bytes_to=p->bytes_to + flow->bytes;
	    data.packets_to=p->packets_to + flow->packets;
	    data.flows++;
	    data.bi=1;
            symbol_hash_insert(ip_hash, &key, &data);
	}	
    } else {		/* existing flow found, increment counts */
	memcpy(&data,p,sizeof(data));
 	data.lasttime= flow->timestamp;
	data.bytes_from=p->bytes_from + flow->bytes;
	data.packets_from=p->packets_from + flow->packets;
	data.flows++;
        symbol_hash_insert(ip_hash, &key, &data);
    }

    return;
}

/* Dump the flow data */
int dump_results()
{
    logger(3,"Writing data to file: %s\n",trace_filename);

  /* Walk the hash, print */

    symbol_hash_map(ip_hash, &stats_write_entry, NULL);

    close_trace_file();

  /* burn the hash */
    symbol_hash_clear(ip_hash);

  /* plant the hash */
    init_hash();

    return;
}

boolean_t stats_write_entry(const void* k, void* d, void* parameters)
{
    const ip_key*       key  = k;
    const ip_data*      data = d;
    tracedetails	trace;

    if(trace_f == NULL) {
        open_trace_file();
    }

    if (trace_f) {
        trace.timestamp=data->firsttime;
        trace.srcIP=key->srcIP;
        trace.dstIP=key->dstIP;
	trace.srcIDB=data->srcIDB;
	trace.dstIDB=data->dstIDB;
        trace.srcport=key->srcport;
        trace.dstport=key->dstport;
        trace.protocol=key->proto;
        trace.packets_to=data->packets_to;
        trace.packets_from=data->packets_from;
        trace.bytes_to=data->bytes_to;
        trace.bytes_from=data->bytes_from;
        trace.srcAS=data->srcAS;
        trace.dstAS=data->dstAS;
        trace.bi=data->bi;
        trace.flows=data->flows;
        trace.communities_len=data->communities_len;
        trace.community1=data->community1;
        trace.community2=data->community2;
        trace.community3=data->community3;
        trace.community4=data->community4;


	if  (bgp_communities==TRUE) 
            fwrite(&trace, sizeof(tracedetails), 1,trace_f);
	else
            fwrite(&trace, 46, 1,trace_f);
    }
    return;
}
