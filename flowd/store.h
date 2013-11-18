#ifndef STORE_H

#define STORE_H

#define TRUE 1
#define FALSE 0

extern  int trace_count;
extern  int max_trace;
extern  char *trace_dir;

extern  int debug_count;
extern  int max_debug;
extern  char *debug_dir;
extern int debug_flows;

#include <stdio.h>

/* typedef ulong ipaddrtype; */
/* typedef uchar_t uchar; */

#include <stdarg.h>
#include <errno.h>
#include "flowdata.h"
#include "flowd.h"

extern errno;

char    trace_filename[PATH_MAX+1];
char    debug_filename[PATH_MAX+1];
char    notrace_file[PATH_MAX+1];

/* The hash structures. The key ... */
typedef struct {
    uint32_t            router;
    uint32_t            srcIP;
    uint32_t            dstIP;
    uint16_t            srcport;
    uint16_t            dstport;
    uint16_t            proto;
} ip_key;

/* ... and the data we're storing */
typedef struct {
    time_t    firsttime;    /* First time flow seen */
    time_t    lasttime;	    /* Last time flow seen */
    uint64_t  packets_to;	    
    uint64_t  packets_from;
    uint64_t  bytes_to;	    
    uint64_t  bytes_from;
    uint64_t  community;
    uint16_t  srcAS;
    uint16_t  dstAS;
    uint16_t  srcIDB;
    uint16_t  dstIDB;
    uint16_t  flows;	/* Number of flows for this one */
    uint16_t  bi;	/* 0=uni-directional, 1=bi-directional */
    uint16_t  communities_len;
    uint32_t  community1;
    uint32_t  community2;
    uint32_t  community3;
    uint32_t  community4;
} ip_data;

#endif
