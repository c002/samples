#ifndef FLOWD_H
/*
 * flowd.h
 *
 */
#define FLOWD_H

#include <sys/types.h>

/* 
 * The default UDP port to listen to 
 */
#define DEFAULT_NETFLOW_PORT 9992

/* Data file signature */
#define SIG1	'F'
#define SIG2	'O'
#define SIG3	'O'
#define VERSION 10	/* written in header of datafiles */

/*
 * This is the condensed details of a single flow
 */
typedef struct
{
    ulong_t   router;      /* IP Address of the router that sent this */
    time_t    timestamp;   /* From unix_secs in the netflow header */
    ulong_t   srcIP;       /* Source IP address */
    ulong_t   dstIP;       /* Destination IP address */
    ushort_t  srcIDB;      /* Source interface index */
    ushort_t  dstIDB;      /* Destination interface index */
    ushort_t  srcport;     /* Source port */
    ushort_t  dstport;     /* Destination port */
    ushort_t  protocol;    /* Protocol number */
    ulong_t   packets;     /* Number of packets in the flow */
    ulong_t   bytes;       /* Number of bytes in the flow */
    ulong_t   inencaps;    /* Number of encapsulation bytes on input */
    ulong_t   outencaps;   /* Number of encapsulation bytes on output */
    ushort_t  srcAS;       /* Peer AS of source address */
    ushort_t  dstAS;       /* Peer AS of destination address */
    uchar_t   tos;	   /* IP Type Of Service */
    ulong_t	proxy_ip;	/* used only for cache - downstream proxy */
} flowdetails;

typedef struct
{
    char  sig1;	   		   /* File signature */
    char  sig2;	   		   /* File signature */
    char  sig3;	   		   /* File signature */
    unsigned char  version;	   /* 10 = 1.0, 44=4.4,  200=20.0  */
    char  options[4];	   	   /* options affecting size of struct */

} __attribute__ ((packed)) header;

/*
 * A convenience struct for the purpose of the trace files
 */
typedef struct
{
    time_t    timestamp;   /* From unix_secs in the netflow header */
    ulong_t   srcIP;       /* Source IP address */
    ulong_t   dstIP;       /* Destination IP address */
    ulong_t   packets_to;  /* Number of packets in the flow */
    ulong_t   packets_from;/* Number of packets in the flow */
    ulong_t   bytes_to;    /* Number of bytes in the flow */
    ulong_t   bytes_from;  /* Number of bytes in the flow */
    ushort_t  srcIDB;	   /* Source interface */
    ushort_t  dstIDB;	   /* Destination interface */
    ushort_t  srcport;     /* Source port */
    ushort_t  dstport;     /* Destination port */
    ushort_t  protocol;    /* Protocol number */
    ushort_t  srcAS;       /* Peer AS of source address */
    ushort_t  dstAS;       /* Peer AS of destination address */
    ushort_t  bi;          /* 1=bi-directional , 0=Uni-directional */
    ushort_t  flows;       /* Number of flows for this one */
			   /* size = 46 */
    ushort_t  communities_len;
    uint32_t  community1;   
    uint32_t  community2;   
    uint32_t  community3;   
    uint32_t  community4;   
			   /* size=64 */
}  __attribute__ ((packed)) tracedetails;


#endif

