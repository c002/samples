/***
 *
 * netflow/bgp debug reader  
 * Takes data collected by 'flowdump' and outputs in text readable form
 *
 * $Id: flowread.c,v 1.1.1.1 2004/05/29 09:06:41 harry Exp $
 * $Source: /usr/local/cvsroot/netflow/netflow/flowd/flowread.c,v $
 *
 ***/

#include <stdio.h>
#include <time.h>
#include <flowd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>

/* Collect total bytes by protocol */
typedef struct {
    unsigned long bytes_to;
    unsigned long bytes_from;
} total_prot;

uint32_t masks[33] = {
    0x00000000,                                         /* /0 ! */
    0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,     /* /1, /2, /3, /4 */
    0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,     /* /5, /6, /7, /8 */
    0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,     /* /9, /10, /11, /12 */
    0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,     /* /13, /14, /15, /16 */
    0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,     /* /17, /18, /19, /20 */
    0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,     /* /21, /22, /23, /24 */
    0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,     /* /25, /26, /27, /28 */
    0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff      /* /29, /30, /31, /32 */
};

unsigned int getmask(char *str);
unsigned int ip2int(char *str);


char *ip2str(int ip, char *ipstr);
char *com2str(int community, char *comstr);
void usage();

char bi[]="<->";		/* for bi-rectional flows */
char uni[]="->";		/* For uni-directional flows */
char *myname;
int TO=0, FROM=1;

main(int argc, char *argv[])
{

    FILE *f1;
    char *protostr;
    int c, n,i, direction;
    tracedetails trace;
    char sip[32], dip[32], t[64], *dir, proto[32];
    char dstport[64], srcport[64], comstr[256], cstr[32];
    int comlen;
    struct tm *tms;
    header head;
    char opstr[5]={0,0,0,0,0};
    char searchnet[32]={0};
    char *fname=NULL;
    int bgp_communities=0, flow_size, mask=32;
    unsigned int matchip=0, port;
    int dototal=0;
    total_prot tcpprots[1024], udpprots[1024];
    total_prot *prots;

    memset(tcpprots, 0, sizeof(tcpprots));
    memset(udpprots, 0, sizeof(udpprots));

    myname = argv[0];
    while ((c = getopt(argc, argv, "s:f:xh")) != EOF)
    {
        switch (c) {
        case 's':
            strncpy(searchnet, optarg, sizeof(searchnet));
	    mask=getmask(searchnet);
	    matchip= ip2int(searchnet);
            break;
        case 'f':
            fname=strdup(optarg);
            break;
        case 'x':
	    dototal=1;
	    break;
	default:
            usage();
        }

    } 


    if (! fname) 
	usage();

/*    if ( !(f1=fopen(argv[1],"r")) ) { */
    if ( !(f1=fopen(fname,"r")) ) {

	fprintf(stderr, "Error opening file %s\n",argv[1]);
	exit(-1);
    }

/* grab header */
    if (! (n=fread(&head, sizeof(head), 1, f1))) {
	fprintf(stderr,"Error reading data file.\n");
	exit(-1);
    }

    if (head.sig1!=SIG1 || head.sig2!=SIG2 || head.sig3!=SIG3) {
	fprintf(stderr,"Error reading data file. Unrecognized file type.\n");
	exit(-1);
    }
	
    for (i=0;i<4;i++) {
	if (head.options[i]!='\0') 
		switch (head.options[i]) {
		    case 'c': bgp_communities=1;
			      break;
		}
		opstr[i]=head.options[i];
    }

    printf("Version=[%d] , Options=[%s]\n",head.version, opstr);

    if (bgp_communities) {
        flow_size=sizeof(trace);
    } else
        flow_size=46;

    while ( (n=fread(&trace, flow_size, 1, f1))) {
	if ((matchip) && ( ((trace.srcIP & mask) != matchip) && 
			   ((trace.dstIP & mask) != matchip)) )
	    continue;

        ip2str(trace.srcIP,sip); 
  	ip2str(trace.dstIP,dip);
	comstr[0]='\0';

	if (bgp_communities) {
	    comlen=0;
	    if (trace.communities_len>0) {
	        comlen = trace.communities_len>4? 4 : trace.communities_len;
	        if (comlen>0) {
	            com2str(trace.community1, cstr);
		    strcat(comstr, cstr);	
	        }
	        if (comlen>1) {
	            com2str(trace.community2, cstr);
		    strcat(comstr, ",");	
		    strcat(comstr, cstr);	
	        }
	        if (comlen>2) {
	            com2str(trace.community3, cstr);
		    strcat(comstr, ",");	
		    strcat(comstr, cstr);	
	        }
	        if (comlen>3) {
	            com2str(trace.community4, cstr);
		    strcat(comstr, ",");	
		    strcat(comstr, cstr);	
	        }
	    }
	}



	if (trace.bi==0) 
	    dir=uni;
	else
	    dir=bi;


	switch (trace.protocol) {
	  case 1: { strcpy(proto,"icmp");
		    break;
		  }
	  case 2: { strcpy(proto,"igmp");
		    break;
		  }
	  case 6: { strcpy(proto,"tcp");
		    break;
		  }
	  case 17: { strcpy(proto,"udp");
		     break;
		   }
	  case 47: { strcpy(proto,"gre");
		     break;
		   }
	  default: sprintf(proto,"%d",trace.protocol);
	}

        if (trace.protocol==1) {
            sprintf(proto,"icmp=0x%x",trace.dstport);
            strcpy(srcport,"");
            strcpy(dstport,"");
        } else {
            sprintf(srcport,":%d",trace.srcport);
            sprintf(dstport,":%d",trace.dstport);


	    for (i=0; i<2; i++ ) {
		if (i==0) {
		    port=trace.srcport;
		    direction=FROM;
		} else {
		    port = trace.dstport;
		    direction=TO;
		}
	        if (dototal && (port<1024 && port>0) && (trace.protocol==6 || trace.protocol==17)) {
		    if (trace.protocol==6) 
			prots=&tcpprots[port];
		    else
			prots=&udpprots[port];

		    prots->bytes_from= prots->bytes_from + trace.bytes_from; 
		    prots->bytes_to= prots->bytes_to + trace.bytes_to; 
		    

		   /* And do a total as proto 0 */

		    if (trace.protocol==6) {
			    tcpprots[0].bytes_from= tcpprots[0].bytes_from + trace.bytes_from; 
			    tcpprots[0].bytes_to= tcpprots[0].bytes_to + trace.bytes_to; 
		    } else {
			    udpprots[0].bytes_from= udpprots[0].bytes_from + trace.bytes_from; 
			    udpprots[0].bytes_to= udpprots[0].bytes_to + trace.bytes_to; 
		    }
		}
	    }
	}

	tms = localtime(&trace.timestamp);
	strftime(t,sizeof(t)-1,"%d-%b-%Y %H:%M:%S",tms);

	if (bgp_communities) 
            printf ("%s: %s%s (AS%d %s) [%d/%d] %s %s%s (AS%d) [%d/%d] %s [%d->%d] %dx\n",
	        t,
	        sip, srcport, trace.srcAS, comstr, trace.packets_from, trace.bytes_from, dir,
	        dip, dstport, trace.dstAS, trace.packets_to, trace.bytes_to,
	        proto, trace.srcIDB, trace.dstIDB, trace.flows);
	else
       	    printf ("%s: %s%s (AS%d) [%d/%d] %s %s%s (AS%d) [%d/%d] %s [%d->%d] %dx\n",
	        t,
	        sip, srcport, trace.srcAS, trace.packets_from, trace.bytes_from, dir,
	        dip, dstport, trace.dstAS, trace.packets_to, trace.bytes_to,
	        proto, trace.srcIDB, trace.dstIDB, trace.flows);
    }

    fclose(f1);

    if (dototal) {
	for (i=0; i<2; i++) {
	    if (i==0)
	        protostr="tcp";
	    else
	        protostr="udp";
	    for (port=1; port<1024; port++) {
		if (i==0) 
		    prots=&tcpprots[port];
		else
		    prots=&udpprots[port];

		if (prots->bytes_to>0 || prots->bytes_from>0)
	            printf("Protocol=%d (%s) Bytes_to=%ld, Bytes_from=%ld\n", 
				port,
				protostr,
				prots->bytes_to, 
			        prots->bytes_from);
	    }							
	    if (i==0)
                printf("Total tcp Bytes_to=%ld, Bytes_from=%ld\n", 
				tcpprots[0].bytes_to, 
				tcpprots[0].bytes_from);
	    else
                printf("Total udp Bytes_to=%ld, Bytes_from=%ld\n", 
				udpprots[0].bytes_to, 
				udpprots[0].bytes_from);
	 }
    }
}

/***
 * Convert int ip address to dotquad format
 ***/
char *
ip2str(int ip, char *ipstr)
{

    register int p1, p2, p3, p4;
   
    p1=(ip & 0xFF000000) >> 24;
    p2=(ip & 0xFF0000) >> 16;
    p3=(ip & 0xFF00) >> 8;
    p4=(ip & 0xFF);

    sprintf(ipstr,"%d.%d.%d.%d",p1,p2,p3,p4);
    return (ipstr);
}

unsigned int
str2ip()
{

}


void
usage()
{

    printf("usage: %s -f infile [-s searchip [-x]]\n", myname);
    printf("\t-f fname , Input trace file.\n");
    printf("\t-s ipcidr , Output ips matching this ip/mask.\n");
    printf("\t-x , Print total bytes by protocol, (use with -s) \n");
    exit(-1);

}

/***
 * Take a community int and format into typical bgp community style
 ***/
char *com2str(int community, char *comstr)
{
    int comm1, comm2;

    comm1=(0xFFFF0000 & community) >> 16;
    comm2=(0xFFFF & community); 
    sprintf(comstr,"%d:%d",comm1, comm2);
    return comstr;
}


unsigned int
getmask(char *str)
{

    char *p;
    char *mask, *ip;
    int imask;

    p = strchr(str, '/');
    if (p) {
        mask = (p+1);
        *p=0;
        ip=str;
        return ( masks[atoi(mask)] );
    } else {
        return (masks[32]);
    } 
}

unsigned int
ip2int(char *str)
{

    char *p;
    char *mask, *ip;
    int imask;

    p = strchr(str, '/');
    if (p) {
        mask = (p+1);
        *p=0;
        ip=str;
        return(inet_addr(ip));
    } else {
        return(inet_addr(str));
    }
}

