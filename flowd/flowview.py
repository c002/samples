#!/opt/python/bin/python
#
#
# View raw flow data files. These are in the following format:
#
# For ICMP it looks like the type and code fields are put into the 
# dstport field. The most commonly observed are:
#
# So [Type | Code ] 	= 16bit 
#       08 | 0	    	= Echo request
#       0  | 0	    	= Echo reply
#	0B | 0		= TimeExceed TTL
#	03 | 01		= Unreach Host
# 	03 | 0D		= Unreach ?
#	03 | 03		= Unreach Port
#
# Headr in data file
#typedef struct
#{
#    char sig1
#    char sig2
#    char sig3
#    unsigned char  version;        /* 10 = 1.0, 44=4.4,  200=20.0  */
#    char  options[4];              /* options affecting size of struct */
#} __attribute__ ((packed)) header;
#
#
#typedef struct
#{
#    time_t    timestamp;   /* From unix_secs in the netflow header */
#    ulong_t   srcIP;       /* Source IP address */
#    ulong_t   dstIP;       /* Destination IP address */
#    ulong_t   packets_to;  /* Number of packets in the flow */
#    ulong_t   packets_from;/* Number of packets in the flow */
#    ulong_t   bytes_to;    /* 5 Number of bytes in the flow */
#    ulong_t   bytes_from;  /* Number of bytes in the flow */
#    ushort_t  srcIDB;      /* Source interface */
#    ushort_t  dstIDB;      /* Destination interface */
#    ushort_t  srcport;     /* Source port */
#    ushort_t  dstport;     /* 10 Destination port */
#    ushort_t  protocol;    /* Protocol number */
#    ushort_t  srcAS;       /* Peer AS of source address */
#    ushort_t  dstAS;       /* Peer AS of destination address */
#    ushort_t  bi;          /* 1=bi-directional , 0=Uni-directional */
#    ushort_t  flows;       /* 15 flowcount */
#	if option communities
#    ushort_t  communities_len;     /* Number of BGP communities 32-bits */
#    uint32_t  community1;       
#    uint32_t  community2;  
#    uint32_t  community3;  
#    uint32_t  community4;  /* 20 */
#} tracedetails;


import struct, time,sys

protos={ 1 : "icmp", 2 : "igmp", 6 : "tcp", 
        17 : "udp", 46 : "rsvp", 47: "gre" }

def ip2str(ip):
    """Convert number IP to string IP"""

    p1=(ip & 0xFF000000L) >> 24
    p2=(ip & 0xFF0000L) >> 16
    p3=(ip & 0xFF00L) >> 8
    p4=(ip & 0xFF)

    n="%d.%d.%d.%d" % (p1,p2,p3,p4)

    return n

# Struct format of the above struct
FORMAT="v5"

headfmt="cccBcccc"
SIZEOF_HEAD=8

f=open(sys.argv[1],"r")

SKIPHEAD=1

if not SKIPHEAD:
    # read header
    data=f.read(SIZEOF_HEAD)
    head=struct.unpack(headfmt,data)
    for i in range(0,3):
        if head[0]!='F' or head[1]!='O' or head[2]!='O':
	    print "Unrecognized file type."
	    sys.exit(-1)

    opts=""
    bgp_communities=0
    for i in range(4,len(head)):
        if head[i]!='\0':
    	  if head[i]=='c':
    	    FORMAT="v4"
	    opts=opts+head[i]+" "
else:
    head=["F","O","O",1,"c",0,0,0]
    opts="c"
    FORMAT="v4"

if FORMAT=="v4":
    fmt="LLLLLLLHHHHHHHHHHLLLL" 
    SIZEOF_FLOWDETAILS=64
elif FORMAT=="v5":
    fmt="LLLLLLLHHHHHHHHH" 
    SIZEOF_FLOWDETAILS=46

print "Version=[%d], Options=[%s]" % (head[3],opts)
data=1
prevtime=0
while data:
    data=f.read(SIZEOF_FLOWDETAILS)
    flow=struct.unpack(fmt,data)

    if FORMAT=="v4" or FORMAT=="v5":
        t=time.strftime("%d-%b-%Y %H:%M:%S", time.localtime(flow[0]))
        if protos.has_key(flow[11]):
	    proto=protos[flow[11]]
        else:
	    proto=str(flow[11])
	
	if (flow[14]==1):
	    direction="<->"
	else:
	    direction="->"

	if flow[10]==1:
	    proto=proto+"=0x%x" % flow[10]
	    srcport=""
	    dstport=""
	else:
	    srcport=":%d" % flow[9]
	    dstport=":%d" % flow[10]

	if FORMAT=="v4":
	    comstr=""	
	    if flow[16]>0:
	        if flow[16]>4:
		    r=4
	        else:
		    r=flow[16]

	        for i in range(0,r):
	            comm1=(0xFFFF0000L & flow[17+i]) >> 16
	            comm2=0xFFFFL & flow[17+i]
	            comstr=comstr + "%d:%d" %(comm1, comm2)
		    if i!=r-1:
		        comstr=comstr+","

	
            print "%s: %s%s (AS%d %s) [%d/%d] %s %s%s (AS%d) [%d/%d] %s [%d->%d] %dx" %  \
            (t,
             ip2str(flow[1]), srcport, flow[12], comstr,flow[3], flow[5],
	     direction,
             ip2str(flow[2]), dstport, flow[13], flow[4], flow[6],
             proto, flow[7],flow[8],flow[15])

	elif FORMAT=="v5":
            print "%s: %s%s (AS%d) [%d/%d] %s %s%s (AS%d) [%d/%d] %s [%d->%d] %dx" %  \
            (t,
             ip2str(flow[1]), srcport, flow[12], flow[3], flow[5],
	     direction,
             ip2str(flow[2]), dstport, flow[13], flow[4], flow[6],
             proto, flow[7],flow[8],flow[15])


	
