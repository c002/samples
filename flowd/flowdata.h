/* $Id: flowdata.h,v 1.1.1.1 2004/05/29 09:06:41 harry Exp $
 * $Source: /usr/local/cvsroot/netflow/netflow/flowd/flowdata.h,v $
 *------------------------------------------------------------------
 * Flow data export record definitions.
 *
 * March 1997, Darren Kerr and Dave Rowell
 *
 * Copyright (c) 1997-1998 by cisco Systems, Inc.
 * All rights reserved.
 *------------------------------------------------------------------
 * $Log: flowdata.h,v $
 * Revision 1.1.1.1  2004/05/29 09:06:41  harry
 *
 *
 * Revision 1.2  2003/04/03 00:04:20  harryr
 * This version and support tools now running live 3-apr-03 with MPLS_TEST enabled.
 *
 * Revision 1.1.1.1  2002/09/04 23:36:57  harryr
 * Initial. 
 * Fixed makefiles, some cleanups. Tested, 24hr stable, output identical. 
 *
 * Revision 1.1.1.1  2002/09/04 04:09:22  harry
 * initial import
 *
 * Revision 1.3  1998/11/09 03:57:22  mikem
 * Removed trasiling control Ms
 *
 * Revision 1.2  1998/11/09 03:56:40  mikem
 * New version from Cisco
 *
 * Revision 3.1.8.4  1998/05/02 21:47:21  drowell
 * CSCdk04127:  DCEF+Flow needs a way to distinguish between VIPs sequence
 * numbers
 * Branch: FIB_branch
 * Added fields to the export record to show what the datagram came from
 * the RP or a Linecard and for VIPs which slot it came from.
 *
 * Revision 3.1.8.3  1998/03/06 05:24:05  drowell
 * CSCdj22254:  Netflow data export byte counts differ from SNMP
 * Branch: FIB_branch
 * Added an export record format that allows us to see the encaps bytes.
 *
 * Revision 3.1.8.2  1997/08/26 23:35:26  soma
 * Sync FIB branch to V111_12_3_CA tag on ELC branch
 *
 * Revision 3.1.8.1  1997/05/12 21:26:32  drowell
 * CSCdj15053:  Minisync of CSCdj08212 and CSCdi62492 to FIB
 * Branch: FIB_branch
 * Adds version 5 export and flow cache size knob.
 * Revision 3.1.2.2  1997/06/02 03:16:49  drowell
 * CSCdj14607:  SNMP index for unroutable and local flows should be 0 not
 * 0xffff
 * Branch: ELC_branch
 * Changed FLOW_INDEX_UNKNOWN to 0.
 *
 * Revision 3.1.2.1  1997/04/28 22:27:49  drowell
 * CSCdi62492:  flow data export struct needs prefix and AS info
 * Branch: ELC_branch
 * Added version 5 export record format.
 *
 * Revision 3.1  1997/04/28 20:11:45  drowell
 *
 *------------------------------------------------------------------
 * $Endlog$
 */

#ifndef __IPFAST_FLOW_EXPORT_H__
#define __IPFAST_FLOW_EXPORT_H__

#ifndef MAX
#define MAX(a,b) ((a) >= (b) ? (a) : (b))
#endif

/* Index exported when there is no interface */
#define FLOW_INDEX_UNKNOWN 0x0000

/*
 * Defines for the flow switching engine type
 */
enum flow_engine_types {
    FLOW_ENGINE_RP = 0,
    FLOW_ENGINE_VIP
};

typedef struct {
    ushort version;         /* Current version */
    ushort count;           /* The number of records in PDU. */
    ulong  SysUptime;       /* Current time in millisecs since router booted */
    ulong  unix_secs;       /* Current seconds since 0000 UTC 1970 */
    ulong  unix_nsecs;      /* Residual nanoseconds since 0000 UTC 1970 */
} FlowStatHdrV1;

#define MAX_FLOWS_PER_V1_PAK   24

typedef struct {
    ipaddrtype srcaddr;    /* Source IP Address */
    ipaddrtype dstaddr;    /* Destination IP Address */
    ipaddrtype nexthop;    /* Next hop router's IP Address */
    ushort input;          /* Input interface index */
    ushort output;         /* Output interface index */
    
    ulong dPkts;           /* Packets sent in Duration */
    ulong dOctets;         /* Octets sent in Duration. */
    ulong First;           /* SysUptime at start of flow */
    ulong Last;            /* and of last packet of flow */
    
    ushort srcport;        /* TCP/UDP source port number or equivalent */
    ushort dstport;        /* TCP/UDP destination port number or equivalent */
    ushort pad;
    uchar  prot;           /* IP protocol, e.g., 6=TCP, 17=UDP, ... */
    uchar  tos;            /* IP Type-of-Service */
    
    uchar  flags;          /* Reason flow was discarded, etc...  */
    uchar  tcp_retx_cnt;   /* Number of mis-seq with delay > 1sec */
    uchar  tcp_retx_secs;  /* Cumulative seconds between mis-sequenced pkts */
    uchar  tcp_misseq_cnt; /* Number of mis-sequenced tcp pkts seen */
    ulong  reserved;
} IPFlowStatV1;

typedef struct {
    FlowStatHdrV1 header;
    IPFlowStatV1  records[0];
} IPStatMsgV1;

#define MAX_FLOWS_PER_V5_PAK   30

typedef struct {
    ushort version;         /* Current version */
    ushort count;           /* The number of records in PDU. */
    ulong  SysUptime;       /* Current time in millisecs since router booted */
    ulong  unix_secs;       /* Current seconds since 0000 UTC 1970 */
    ulong  unix_nsecs;      /* Residual nanoseconds since 0000 UTC 1970 */
    ulong  flow_sequence;   /* Seq counter of total flows seen */
    uchar  engine_type;     /* Type of flow switching engine */
    uchar  engine_id;       /* ID number of the flow switching engine */
    ushort reserved;
} FlowStatHdrV5;

typedef struct {
    ipaddrtype srcaddr;    /* Source IP Address */
    ipaddrtype dstaddr;    /* Destination IP Address */
    ipaddrtype nexthop;    /* Next hop router's IP Address */
    ushort input;          /* Input interface index */
    ushort output;         /* Output interface index */
    
    ulong dPkts;           /* Packets sent in Duration */
    ulong dOctets;         /* Octets sent in Duration. */
    ulong First;           /* SysUptime at start of flow */
    ulong Last;            /* and of last packet of flow */
    
    ushort srcport;        /* TCP/UDP source port number or equivalent */
    ushort dstport;        /* TCP/UDP destination port number or equivalent */
    uchar  rsvd;
    uchar  tcp_flags;      /* Cumulative OR of tcp flags */
    uchar  prot;           /* IP protocol, e.g., 6=TCP, 17=UDP, ... */
    uchar  tos;            /* IP Type-of-Service */
    ushort src_as;         /* originating AS of source address */
    ushort dst_as;         /* originating AS of destination address */
    uchar  src_mask;       /* source address prefix mask bits */
    uchar  dst_mask;       /* destination address prefix mask bits */
    ushort pad;
} IPFlowStatV5;

typedef struct {
    FlowStatHdrV5 header;
    IPFlowStatV5  records[0];
} IPStatMsgV5;

#define MAX_FLOWS_PER_V6_PAK   27

typedef struct {
    ushort version;         /* Current version */
    ushort count;           /* The number of records in PDU. */
    ulong  SysUptime;       /* Current time in millisecs since router booted */
    ulong  unix_secs;       /* Current seconds since 0000 UTC 1970 */
    ulong  unix_nsecs;      /* Residual nanoseconds since 0000 UTC 1970 */
    ulong  flow_sequence;   /* Seq counter of total flows seen */
    uchar  engine_type;     /* Type of flow switching engine */
    uchar  engine_id;       /* ID number of the flow switching engine */
    ushort reserved;
} FlowStatHdrV6;

typedef struct {
    ipaddrtype srcaddr;    /* Source IP Address */
    ipaddrtype dstaddr;    /* Destination IP Address */
    ipaddrtype nexthop;    /* Next hop router's IP Address */
    ushort input;          /* Input interface index */
    ushort output;         /* Output interface index */
    
    ulong dPkts;           /* Packets sent in Duration */
    ulong dOctets;         /* Octets sent in Duration. */
    ulong First;           /* SysUptime at start of flow */
    ulong Last;            /* and of last packet of flow */
    
    ushort srcport;        /* TCP/UDP source port number or equivalent */
    ushort dstport;        /* TCP/UDP destination port number or equivalent */
    uchar  rsvd;
    uchar  tcp_flags;      /* Cumulative OR of tcp flags */
    uchar  prot;           /* IP protocol, e.g., 6=TCP, 17=UDP, ... */
    uchar  tos;            /* IP Type-of-Service */
    ushort src_as;         /* originating AS of source address */
    ushort dst_as;         /* originating AS of destination address */
    uchar  src_mask;       /* source address prefix mask bits */
    uchar  dst_mask;       /* destination address prefix mask bits */
    uchar  in_encaps;      /* size in bytes of the input encapsulation */
    uchar  out_encaps;     /* size in bytes of the output encapsulation */
    ulong  peer_nexthop;   /* IP address of the nexthop within the peer (FIB)*/
} IPFlowStatV6;

typedef struct {
    FlowStatHdrV6 header;
    IPFlowStatV6  records[0];
} IPStatMsgV6;

#define MAX_V1_FLOW_PAK_SIZE (sizeof(FlowStatHdrV1) + \
			      sizeof(IPFlowStatV1) * MAX_FLOWS_PER_V1_PAK)

#define MAX_V5_FLOW_PAK_SIZE (sizeof(FlowStatHdrV5) + \
			      sizeof(IPFlowStatV5) * MAX_FLOWS_PER_V5_PAK)

#define MAX_V6_FLOW_PAK_SIZE (sizeof(FlowStatHdrV6) + \
			      sizeof(IPFlowStatV6) * MAX_FLOWS_PER_V6_PAK)

#define MAX_FLOW_PAK_SIZE MAX(MAX(MAX_V1_FLOW_PAK_SIZE,  \
				  MAX_V5_FLOW_PAK_SIZE), \
			      MAX_V6_FLOW_PAK_SIZE)

#endif /* __IPFAST_FLOW_EXPORT_H__ */
