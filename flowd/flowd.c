/***
 * 
 * Raw netflow collector 
 *
 * Options: See ./collect -h
 *
 * $Id: flowd.c,v 1.1.1.1 2004/05/29 09:06:41 harry Exp $
 * $Source: /usr/local/cvsroot/netflow/netflow/flowd/flowd.c,v $
 *
 ***/

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <strings.h>

#include <symbol.hash.h>
#include <bgp_api.h>
#include <ip2cidr.h>

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif
#define PORT 9992

#define LOGLEVEL 2	/* default loglevel. 0=Nothing, 2=Informational, 4=Verbose */

int debug_hist=0;
int max_debug=0;
int debug_count=0;
int debug_flows=FALSE;	/* unmolested flows */

int trace_hist=0;
int max_trace=0;
int trace_count=0;

symbol_hash_t*   ip_hash=NULL;
int main_doflush = FALSE;
int zeroint=FALSE;

int flows_in_packet=0;
int rate_limit=999999;		/* huge default, rate limmiting, flows per second  */
int loglevel=LOGLEVEL;

static int flush_interval = 15 * 60;
char *trace_d = "/var/log/netflow/trace";
char *logfile = "/var/log/netflow/trace/flowd.log";
char *trace_dir = NULL;
const char *debug_dir = "/var/log/netflow/trace";
int		bgp_communities=FALSE;
int		interface_indexes=FALSE, sequence=FALSE;
char		options[4]={0,0,0,0};

void            usage(void);
void 		set_alarm();
static void     handle_usrsig(int sig, siginfo_t* sip, void* uap);
static void     signal_init(void);
char		*myname;

main(int argc, char *argv[])
{
    int c,i=0, limitting=FALSE, port=PORT;
    int total_flows=0;
    int nflows;			/* number of flows in a packet */
    extern char *optarg;
    time_t	t,prevt=time(NULL);
    pid_t 	pid, pgrp;
    int 	forkme=TRUE; 

    myname=argv[0];

    while ((c = getopt(argc, argv, "d:f:r:t:l:p:nchxsz")) != EOF)
    {
        switch (c) {
        case 'x':
            forkme=FALSE;
	    break;
        case 'n':			/* throw away flows to iface 0 */
	    zeroint=TRUE;
	    break;
        case 'c':			/* capture BGP communities */
            bgp_communities=TRUE;
	    options[i++]='c';
            break;
        case 'd':			/* storeage dir */
	    trace_dir=strdup(optarg);
            break;
        case 'f':			/* flush data every n-seconds */
            flush_interval = atoi(optarg);
            break;
        case 'h':			
		usage();
        case 'l':			/* logging level */
	    loglevel=atoi(optarg);
            break;
        case 'p':			/* UDP port to listen on */
            port = atoi(optarg);
            break;
        case 's':			/* sequence number check only */
            sequence = TRUE;
	    forkme=FALSE;
	    logfile= "/dev/null";
            break;
        case 'z':			/* No logfile */
	    logfile= "/dev/null";
	    break;
        case 'r':			/* Set rate limitting (DOS protection) */
            rate_limit = atoi(optarg);
            break;
        case 't':			/* Hours to keep */
            if (! (trace_hist = atoi(optarg) * 60 * 60))
                usage();
            max_trace=trace_hist / flush_interval;
            break;
	default:
		usage();
	}
    }

/* Daemon init */

    if (forkme==TRUE) {
        chdir("/");

        if ((pid=fork())>0) 
	    exit(0);
        else if (pid == -1) {
	    fprintf(stderr,"Failed to fork child\n");
	    exit(-1);
        }

        if ((pgrp = setsid()) == -1) {
	    fprintf(stderr,"Failed to become group leader\n");
	    exit(-1);
        }

        if (freopen("/dev/null","r",stdin) == NULL) {
	    fprintf(stderr,"Failed to reopen stdin to devnull\n");
	    exit(-1);
        }
        if (freopen("/dev/null","w",stdout) == NULL) {
	    fprintf(stderr,"Failed to reopen stdout to devnull\n");
	    exit(-1);
        }
    }

    if (!trace_dir)
	trace_dir=trace_d;

    init_logger(logfile);


    if (!sock_init(port)) {
	logger(1,"Failed to initialise socket on port: %d\n",port);
	exit(-1);
    }

    max_trace=trace_hist / flush_interval;

    signal_init();

    if (bgp_communities) { 
        if (bgp_db_attach(argv[0])!=1) {
	    logger(1,"Failed to attach to existing bgp shared mem segment.\n");
	    exit(-1);
	}
    }

    set_alarm();

    logger(2,"Starting Netflow collector ...\n");
    logger(2,"Rate limit=%d flows/sec History=%d seconds, %sflush interval=%d seconds. Produces %d files\n",rate_limit,trace_hist,(zeroint?"Dropping egress iface 0 flows, ":" "), flush_interval, max_trace);

    for(;;) {
        t=time(NULL);
	if (t>prevt) {
	    if (limitting==TRUE)
	        logger(2,"Rate is >=%d flows/sec. Dropping flows.\n",total_flows);
	    prevt=t;
	    logger(4,"nflows=%d\n",total_flows);
	    total_flows=0;
	    flows_in_packet=0;
	}

	if (total_flows < rate_limit) {
	    sock_handle_one_packet(sequence);
	    total_flows+=flows_in_packet;	
	    limitting=FALSE;
	} else
	    limitting=TRUE;

	if (main_doflush && !sequence) {
	    logger(3,"Signal recieved. Flushing data\n");
	    main_doflush=FALSE;
	    dump_results();
	}
    }
}

static void     handle_sigalarm(int sig, siginfo_t* sip, void* uap) {
    main_doflush++;
    set_alarm();
}

void
set_alarm()
{
    time_t              cur_time;
    time_t              next_flush;
    
    cur_time = time(NULL);
    next_flush = ((cur_time / flush_interval) + 1) * flush_interval;
    
    alarm(next_flush - cur_time);
}

static void     handle_usrsig(int sig, siginfo_t* sip, void* uap) {
    
    switch(sig) {
        
    case SIGUSR1: 		/* flush datafile */
        main_doflush = TRUE;
	logger(1,"SIGUSR1 recieved, flushing.\n");
        break;
        
    case SIGUSR2:		/* debug on/off */
        debug_flows = !debug_flows;
	logger(1,"SIGUSR2 recieved, debug is %d\n",debug_flows);
        break;
    }
}

/**
 * ALARM - flush results and rotate files 
 * HUP   - Re-read config file 
 ***/

static void signal_init(void) {

    struct sigaction     sa;
    
    sigemptyset(&sa.sa_mask);
    sa.sa_flags =  0;
    
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = handle_sigalarm;
    sigaction(SIGALRM, &sa, NULL);

    sa.sa_handler = handle_usrsig;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

}

void usage()
{
    printf("usage: %s [-n] [-h] [-t hours] [-f seconds] [-r flows] [-c]\n",myname);
    printf("\t-h = help\n");
    printf("\t-x = Don't fork.\n");
    printf("\t-t hours = Hours of history to keep before recycling data files.\n");
    printf("\t-f seconds = Flush interfal to dump data to next data file.\n");
    printf("\t-p port = UDP port to listen on for netflow data.\n");
    printf("\t-r flows = Rate limitting. Max number of flows per second before throwing away.\n");
    printf("\t-s Print NetFlow packet headers. Used to check sequence numbers. Wont fork.\n");
    printf("\t-c = Include upto 4 BGP communities on source address (increases data file size by 18 bytes per flow)\n");
    printf("\t-z =  No logfile\n");
    printf("\t-n = Throw away flows to interface 0. Router rate limmited.\n");
    exit(0);
}

