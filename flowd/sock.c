/*
 * sock.c
 *
 * Neflow listener for the 
 * BTA V4 Netflow collector
 *
 * Author: Mike McCauley (mikem@open.com.au)
 * Copyright (C) 1998 connect.com.au pty ltd
 * $Id: sock.c,v 1.1.1.1 2004/05/29 09:06:42 harry Exp $
 */

#ifndef lint
static char vcid[] = "$Id: sock.c,v 1.1.1.1 2004/05/29 09:06:42 harry Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector.h>

#include "sock.h"
#include "flowdata.h"
#include "flowd.h"
#include "store.h"

#define MODULE_NAME "sock"

/* This is the socket we listen on for Netflow UDP packets */
static int      sock;

/* This is set to an error message whenever a function fails */
static char     last_errmsg[1000] = "";

/* Forward declarations */
void sock_dispatch_v1_packet(ulong_t from, IPStatMsgV1* fp);
void sock_dispatch_v5_packet(ulong_t from, IPStatMsgV5* fp);
void sock_dispatch_v6_packet(ulong_t from, IPStatMsgV6* fp);
void sock_check_packet(ulong_t router, ulong_t sequence, uchar engine_type, 
		       uchar engine_id, ushort_t count, ulong sysuptime);
void print_sequence(ulong_t router, FlowStatHdrV6 *);

router* sock_find_router(ulong_t address, uchar engine_type, uchar engine_id);
const char*		sock_last_errmsg(void);
int			sock_handle_one_packet(int);

extern int flows_in_packet;
extern int zeroint;

/* Internal variables */
vector_t* routers;

/*
 * sock_init
 * Initialise this module
 * Return TRUE if successful
 */
int
sock_init(int port)
{
    struct   sockaddr_in sockaddr;
    int      optval;

    /* Create a socket */
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) 
    {
	sprintf(last_errmsg, "socket call failed: %s", strerror(errno));
	return FALSE;
    }
  
    /* Give it a big buffer */
    optval = SOCK_SOCKET_BUFFER_SIZE;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, 
		   (void *)&optval, sizeof(optval)) != 0)
    {
	fprintf(stderr,"sock: %s\n",strerror(errno));

	/* Try to get _something_ better than the default! */
	optval = SOCK_SOCKET_BUFFER_SIZE_FALLBACK;
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, 
		   (void *)&optval, sizeof(optval));
    }
  
    /*
     * Bind the socket to our port numner
     */
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&sockaddr, 
	     sizeof(struct sockaddr_in)) < 0) 
    {
	sprintf(last_errmsg, "bind call failed: %s",  strerror(errno));
	return FALSE;
    }
   
    /* Make an empty vector to remember the routers we have seen */
    routers = vector_create(sizeof(router), 0);

    return TRUE; /* Success */
}

const char*
sock_last_errmsg()
{
    return last_errmsg;
}

/*
 * sock_handle_one_packet
 * Read one netflow packet from the incoming socket
 * and dispatch it. Blocks until a packet is received.
 * Returns TRUE if a packet was received, FALSE otherwise.
 * We unpack the packet in different ways depending on which
 * version we have received.
 */
int
sock_handle_one_packet(int sequence)
{
    static char buf[MAX_FLOW_PAK_SIZE];
    struct sockaddr_in from;
    int                fromlen = sizeof(from);
    int                plen;      /* Packet length */

    if ((plen = recvfrom(sock, buf, MAX_FLOW_PAK_SIZE, 0, 
			 (struct sockaddr *)&from, &fromlen)) > 0) 
    {
	ulong_t router = ntohl(from.sin_addr.s_addr);
	/* Find out what type of packet it is */
	if (ntohs(((IPStatMsgV1 *)buf)->header.version) == FLOWSTATHDR_VERSION_1
	    && ntohs(((IPStatMsgV1 *)buf)->header.count) <= MAX_FLOWS_PER_V1_PAK)
	{
	    sock_dispatch_v1_packet(router, (IPStatMsgV1*)buf);
	}
	else if (ntohs(((IPStatMsgV5 *)buf)->header.version) == FLOWSTATHDR_VERSION_5
	         && ntohs(((IPStatMsgV5 *)buf)->header.count) <= MAX_FLOWS_PER_V5_PAK)
	{
	    sock_dispatch_v5_packet(router, (IPStatMsgV5*)buf);
	}
	else if (ntohs(((IPStatMsgV6 *)buf)->header.version) == FLOWSTATHDR_VERSION_6
	         && ntohs(((IPStatMsgV6 *)buf)->header.count) <= MAX_FLOWS_PER_V6_PAK)
	{

	    if (sequence==TRUE) {
	 	print_sequence(router, (FlowStatHdrV6*)buf);
	    } else {
		sock_dispatch_v6_packet(router, (IPStatMsgV6*)buf);
	    }
	}
	else
	{
	    sprintf(last_errmsg, "Bogus netflow packet received\n");
	    return FALSE;
	}
	return TRUE;
    }
    else
    {
	return FALSE; /* Nothing there, perhaps a signal? */
    }
}

/*
 * sock_dispatch_v1_packet
 * Unpack a V1 packet and send it to rules for handling
 */
void
sock_dispatch_v1_packet(ulong_t from, IPStatMsgV1* fp)
{
    flowdetails  flowdetails;
    int          i;
	    
    sock_check_packet(from, 
		      0,
		      0,
		      0,
		      0,
		      ntohl(fp->header.SysUptime));
    flows_in_packet=ntohs(fp->header.count);

    for (i = 0; i < ntohs(fp->header.count); i++)
    {
        if (zeroint && ( ntohs(fp->records[i].output) == 0) )
	    continue;

	flowdetails.router    = from;
	flowdetails.timestamp = ntohl(fp->header.unix_secs);
	flowdetails.srcIP     = ntohl(fp->records[i].srcaddr);
	flowdetails.dstIP     = ntohl(fp->records[i].dstaddr);
	flowdetails.srcIDB    = ntohs(fp->records[i].input);
	flowdetails.dstIDB    = ntohs(fp->records[i].output);
	flowdetails.srcport   = ntohs(fp->records[i].srcport);
	flowdetails.dstport   = ntohs(fp->records[i].dstport);
	flowdetails.protocol  = fp->records[i].prot;
	flowdetails.packets   = ntohl(fp->records[i].dPkts);
	flowdetails.bytes     = ntohl(fp->records[i].dOctets);
	flowdetails.inencaps  = 0;
	flowdetails.outencaps = 0;
	flowdetails.srcAS     = 0;
	flowdetails.dstAS     = 0;
	flowdetails.tos	      = fp->records[i].tos;
	flowdetails.proxy_ip  = 0;
	
	handle_flow(&flowdetails);
    }
}

/*
 * sock_dispatch_v5_packet
 * Unpack a V5 packet and send it to rules for handling
 */
void
sock_dispatch_v5_packet(ulong_t from, IPStatMsgV5* fp)
{
    flowdetails  flowdetails;
    int          i;
    
    sock_check_packet(from, 
		      ntohl(fp->header.flow_sequence),
		      0,
		      0,
		      ntohs(fp->header.count),
		      ntohl(fp->header.SysUptime));
    flows_in_packet=ntohs(fp->header.count);

    for (i = 0; i < ntohs(fp->header.count); i++)
    {
        if (zeroint && ( ntohs(fp->records[i].output) == 0) )
	    continue;

	flowdetails.router    = from;
	flowdetails.timestamp = ntohl(fp->header.unix_secs);
	flowdetails.srcIP     = ntohl(fp->records[i].srcaddr);
	flowdetails.dstIP     = ntohl(fp->records[i].dstaddr);
	flowdetails.srcIDB    = ntohs(fp->records[i].input);
	flowdetails.dstIDB    = ntohs(fp->records[i].output);
	flowdetails.srcport   = ntohs(fp->records[i].srcport);
	flowdetails.dstport   = ntohs(fp->records[i].dstport);
	flowdetails.protocol  = fp->records[i].prot;
	flowdetails.packets   = ntohl(fp->records[i].dPkts);
	flowdetails.bytes     = ntohl(fp->records[i].dOctets);
	flowdetails.inencaps  = 0;
	flowdetails.outencaps = 0;
	flowdetails.srcAS     = ntohs(fp->records[i].src_as);
	flowdetails.dstAS     = ntohs(fp->records[i].dst_as);
	flowdetails.tos	      = fp->records[i].tos;
	flowdetails.proxy_ip  = 0;
	
	handle_flow(&flowdetails);
    }
}

/*
 * sock_dispatch_v6_packet
 * Unpack a V6 packet and send it to rules for handling
 */
void
sock_dispatch_v6_packet(ulong_t from, IPStatMsgV6* fp)
{
    flowdetails  flowdetails;
    int          i;
	   
    sock_check_packet(from, 
		      ntohl(fp->header.flow_sequence),
		      fp->header.engine_type,
		      fp->header.engine_id,
		      ntohs(fp->header.count),
		      ntohl(fp->header.SysUptime));
   
    flows_in_packet=ntohs(fp->header.count);
 
    for (i = 0; i < ntohs(fp->header.count); i++) 
    {
        if (zeroint && ( ntohs(fp->records[i].output) == 0) )
	    continue;

	flowdetails.router    = from;
	flowdetails.timestamp = ntohl(fp->header.unix_secs);
	flowdetails.srcIP     = ntohl(fp->records[i].srcaddr);
	flowdetails.dstIP     = ntohl(fp->records[i].dstaddr);
	flowdetails.srcIDB    = ntohs(fp->records[i].input);
	flowdetails.dstIDB    = ntohs(fp->records[i].output);
	flowdetails.srcport   = ntohs(fp->records[i].srcport);
	flowdetails.dstport   = ntohs(fp->records[i].dstport);
	flowdetails.protocol  = fp->records[i].prot;
	flowdetails.packets   = ntohl(fp->records[i].dPkts);
	flowdetails.bytes     = ntohl(fp->records[i].dOctets);
	flowdetails.inencaps  = fp->records[i].in_encaps;
	flowdetails.outencaps = fp->records[i].out_encaps;
	flowdetails.srcAS     = ntohs(fp->records[i].src_as);
	flowdetails.dstAS     = ntohs(fp->records[i].dst_as);
	flowdetails.tos	      = fp->records[i].tos;
	flowdetails.proxy_ip  = 0;
	
	handle_flow(&flowdetails);
    }
}

/*
 * sock_check_packet
 * Check that the sequence number is the one we are expecting to get
 * from that router and engine_id.
 * Also check if the sys uptime went backwards too, and raise an exception
 * After checking, readjust it to be the next one we expect (ie the
 * the current seq number + the number of flows in this one)
 */
void
sock_check_packet(ulong_t from, ulong_t sequence, uchar engine_type, 
		  uchar engine_id, ushort_t count,
		  ulong sysuptime)
{
    /* Find some info about this router */
    router* p = sock_find_router(from, engine_type, engine_id);

    /* Dont complain if its the first flow weve seen */
    if (p->next_seq > 0 && sequence != p->next_seq) {
	logger(3,"Flows lost (%lu) - expected %lu got %lu\n", sequence - p->next_seq, p->next_seq, sequence);
	if (sequence < p->next_seq && sysuptime < p->sysuptime)  {
/*
	    stats_flush_ip("netflow", time(NULL));
	    stats_flush_if("ifsummary", time(NULL));
	    stats_flush_altc("alt-tag-counters", time(NULL));
	    fr_reboot_router(from);
*/
	}
    }
    p->next_seq = sequence + count;
    p->sysuptime = sysuptime;
}

void print_sequence(ulong_t router, FlowStatHdrV6 *head)
{

    printf("%lu %lu %d %d/%d (version=%d) \n", router,
				 	   head->flow_sequence, 
					   head->count, 
					   head->engine_id, 
					   head->engine_type, 
					   head->version);
    return;
}

/*
 * sock_find_router
 * Find and return the address of the router structure for the router 
 * with the given IP address 
 */
router*
sock_find_router(ulong_t address, uchar engine_type, uchar engine_id)
{
    router		new;
    unsigned int	i;

    /* Linear search. Assumes there will be very few routers per meter box */
    for (i = 0; i < vector_length(routers); i++)
    {
	router* r = (router*)vector_get(routers, i);
	if (r->routip == address && r->engine_type == engine_type && r->engine_id == engine_id)
	    return r;
    }
    
    /* Not found, create a new one and add it to the vector */
    new.routip = address;
    new.engine_type = engine_type;
    new.engine_id = engine_id;
    new.next_seq = 0;
    routers = vector_append(routers, &new); /* Might get moved */
    return (router*)vector_get(routers, i);
}

