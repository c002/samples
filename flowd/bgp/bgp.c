/* -*-mode: c; fill-column: 75; comment-column: 50; -*- */
/*
** 	$Id: bgp.c,v 1.1.1.1 2004/05/29 09:06:46 harry Exp $	 
**
** (c) Copyright 1998 Connect.com.au Pty. Ltd. and Unicity Pty. Ltd.
**
** Jim.Crumpler@unicity.com.au
**
** A partial implementation of Border Gateway Protocol Version 4.
** 
** The code implements the reading part of BGP4, that is, it can
** receive route updates from a BGP4 peer, but cannot update routes
** itself.
** 
** Refer to RFC1771 (BGP4) and RFCxxxx (BGP4 communities atttributes
** extension).
**
**
*/


#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <signal.h>

#include "bgp_api.h"
#include "ip2cidr.h"
#include "mpmalloc.h"

#include "exception.h"
#include "btalib.h"

#define DEFAULT_BASE_DIR "/var/log"

#define BGP_PID_FILE "/var/run/bgp.pid"

/*
** The number of seconds to sleep for when we're idle.
*/
#define BGP_POLL_LOOP_IDLE_SLEEP_TIME	2

/*
** The number of idle loops after receiving some updates
** that we consider the initial update flood to be completed.
*/
#define BGP_FLOOD_END_IDLE_COUNT	4


#define BGP_HEADER_LENGTH		19	  /* protocol */
#define BGP_READBUF_MAX			32768	  /* the largest message we can handle */
#define BGP_DEFAULT_PORT		179
#define BGP_DEFAULT_TIMER_HOLDTIME	180
#define BGP_DEFAULT_TIMER_CONNECT_RETRY	30
#define BGP_DEFAULT_TIMER_KEEPALIVE	(BGP_DEFAULT_TIMER_HOLDTIME / 3)

/*
** Number of seconds between watchdog ticks
*/
#define BGP_DEFAULT_TIMER_WATCHDOG	60

/*
** 
*/
#define BGP_ASPATH_STORE_SIZE		1024;

/*
**
*/
#define BGP_COMMUNITY_STORE_SIZE	1024;

/* 
** All packet transmitted <prefix,length) route tupples are variable
** length.
*/
#define BGP_PREFIX_STORAGE_SIZE(prefix) (((prefix - 1) / 8) + 1) 

/*
** AS PATHs actually built from SETs and/or SEQUENCES
*/
enum bgp_as_path_segment_type_e {
    bgp_AS_PATH_SET = 1,
    bgp_AS_PATH_SEQUENCE = 2,
    bgp_max_AS_PATH_TYPE = 2,
};

static const char*	bgp_as_path_segment_type_names[] = {"None - Error",
							    "AS_SET",
							    "AS_SEQUENCE"};

/*
** NOTIFICATION message types.
*/ 
enum bgp_notify_e {
    bgp_error_min_NOTIFY	= 0,
    bgp_NOTIFY_MSG_HEADER	= 1,
    bgp_NOTIFY_MSG_OPEN		= 2,
    bgp_NOTIFY_MSG_UPDATE	= 3,
    bgp_NOTIFY_HOLD_TIMER	= 4,
    bgp_NOTIFY_FSM		= 5,
    bgp_NOTIFY_CEASE		= 6,
    bgp_max_NOTIFY		= 6,
};

enum bgp_nofify_sub_e {
    bgp_min_NOTIFY_SUB			= 1,
    bgp_NOTIFY_HEADER_LOST_SYNC		= 1,
    bgp_NOTIFY_HEADER_BAD_LEN		= 2,
    bgp_NOTIFY_HEADER_BAD_TYPE		= 3,
    bgp_max_NOTIFIY_SUB_HEADER		= 3,
    
    bgp_NOTIFY_OPEN_BAD_VER		= 1,
    bgp_NOTIFY_OPEN_BAD_AS		= 2,
    bgp_NOTIFY_OPEN_BAD_ID		= 3,
    bgp_NOTIFY_OPEN_BAD_OPT		= 4,
    bgp_NOTIFY_OPEN_BAD_BGP_IDENT	= 5,
    bgp_NOTIFY_OPEN_AUTH_FAILURE	= 6,
    bgp_max_NOTIFIY_SUB_OPEN		= 6,
    
    bgp_NOTIFY_UPDATE_MAFORMED_ATTR		= 1,
    bgp_NOTIFY_UPDATE_UNKNOWN_WELL_KNOWN_ATTR	= 2,
    bgp_NOTIFY_UPDATE_MISSING_WELL_KNOWN_ATTR	= 3,
    bgp_NOTIFY_UPDATE_ATTR_FLAGS		= 4,
    bgp_NOTIFY_UPDATE_ATTR_LENGTH		= 5,
    bgp_NOTIFY_UPDATE_INVALID_ORIGIN		= 6,
    bgp_NOTIFY_UPDATE_AS_ROUTE_LOOP		= 7,
    bgp_NOTIFY_UPDATE_INVALID_NEXT_HOP		= 8,
    bgp_NOTIFY_UPDATE_OPTIONAL_ATTR_ERROR	= 9,
    bgp_NOTIFY_UPDATE_INVALID_NETWORK		= 10,
    bgp_NOTIFY_UPDATE_MALFORMED_AS_PATH		= 11,
    bgp_max_NOTIFY_SUB_UPDATE			= 11
};

static const char*	bgp_origin_names[] = {"IGP",
					      "EGP",
					      "INCOMPLETE"};


/*
** Internal Finite State Machine states,
** As exmaple in RFC1771
**
*/ 
enum bgp_state_e {
    bgp_min_STATE		= 1,
    bgp_STATE_IDLE		= 1,
    bgp_STATE_CONNECT		= 2,
    bgp_STATE_ACTIVE		= 3,
    bgp_STATE_OPENSENT		= 4,
    bgp_STATE_OPENCONFIRM	= 5,
    bgp_STATE_ESTABLISHED	= 6,
    bgp_max_STATE		= 6
};

/*
** BGP Events.
**
*/
enum bgp_event_e {
    bgp_min_EVENT			= 1,
    bgp_EVENT_START			= 1,
    bgp_EVENT_STOP			= 2,
    bgp_EVENT_TRANSPORT_OPEN		= 3,
    bgp_EVENT_TRANSPORT_CLOSED		= 4,
    bgp_EVENT_TRANSPORT_OPEN_FAILED	= 5,
    bgp_EVENT_TRANSPORT_FATAL		= 6,
    bgp_EVENT_RETRY_TIMER_EXPIRE	= 7,
    bgp_EVENT_HOLD_TIMER_EXPIRE		= 8,
    bgp_EVENT_KEEPALIVE_TIMER_EXPIRE	= 9,
    bgp_EVENT_RX_OPEN			= 10,
    bgp_EVENT_RX_KEEPALIVE		= 11,
    bgp_EVENT_RX_UPDATE			= 12,
    bgp_EVENT_RX_NOTIFICATION		= 13,
    bgp_EVENT_IDLE			= 14,
    bgp_EVENT_TERMINATE			= 15,
    bgp_max_EVENT			= 15

};

static const char*	bgp_event_names[] = {"event None - Error",
					     "start",
					     "stop",
					     "transport_open",
					     "transport_closed",
					     "transport_open_failed",
					     "tranport_fatal",
					     "connect_retry_expire",
					     "hold_timer_expire",
					     "keepalive_timer_expire",
					     "rx_open",
					     "rx_keepalive",
					     "rx_update",
					     "rx_notification",
					     "idle",
					     "terminate"};


static const char*	bgp_state_names[] = {"state None - Error",
					     "Idle",
					     "Connect",
					     "Active",
					     "OpenSent",
					     "OpenConfirm",
					     "Established"};

/*
** The actual message types 
*/
enum bgp_msg_type_e {
    bgp_min_MSG			= 1,
    bgp_MSG_OPEN		= 1,
    bgp_MSG_UPDATE		= 2,
    bgp_MSG_NOTIFICATION	= 3,
    bgp_MSG_KEEPALIVE		= 4,
    bgp_max_MSG			= 4
};

static const char*	bgp_msg_type_names[] = {"None - Error",
						"Open",
						"Update",
						"Notification",
						"Keepalive"};


/*
** Path Attribute types.
*/
enum bgp_pa_type_e {
    bgp_min_PA			= 1,
    bgp_PA_ORIGIN		= 1, /* RFC1771 */
    bgp_PA_AS_PATH		= 2, /* RFC1771 */
    bgp_PA_NEXT_HOP		= 3, /* RFC1771 */
    bgp_PA_MULTI_EXIT_DISC	= 4, /* RFC1771 */
    bgp_PA_LOCAL_PREF		= 5, /* RFC1771 */
    bgp_PA_ATOMIC_AGGREGATE	= 6, /* RFC1771 */
    bgp_PA_AGGREGATOR		= 7, /* RFC1771 */
    bgp_PA_COMMUNITIES		= 8, /* RFC1997 */
    bgp_PA_ORIGINATOR_ID	= 9, /* cisco */
    bgp_PA_CLUSTER_LIST		= 10, /* cisco */
    bgp_PA_DESTINATION_PREFERENCE = 11, /* MCI */
    bgp_PA_ADVERTISER		= 12, /* Bay */
    bgp_PA_RCID_PATH		= 13, /* Bay */
    bgp_max_PA			= 13
    
};

static const char*	bgp_pa_type_names[] = {"None - Error",		/* 0 */
					       "ORIGIN",		/* 1 */
					       "AS_PATH",		/* 2 */
					       "NEXT_HOP",		/* 3 */
					       "MULTI_EXIT_DISC",	/* 4 */
					       "LOCAL_PREF",		/* 5 */
					       "ATOMIC_AGGREGATE",	/* 6 */
					       "AGGREGATOR",		/* 7 */
					       "COMMUNITIES",		/* 8 */
					       "ORIGINATION_ID",	/* 9 */
					       "CLUSTER_LIST",		/* 10 */
					       "DESTINATION_PREFERENCE", /* 11 */
					       "ADVERTISER",		/* 12 */
					       "RCID_PATH"		/* 13 */
};


enum bgp_version_e {
    bgp_VERSION_4 = 4
};

enum bgp_flood_e {
    bgp_FLOOD_NOT_STARTED	= 0,
    bgp_FLOOD_STARTED,
    bgp_FLOOD_COMPLETED,
    bgp_max_FLOOD		= bgp_FLOOD_COMPLETED
};

/*
**   ----------------------------------------
*/
struct bgp_prefix {
    uint8_t		length;
    uint32_t		prefix;
};

struct bgp_tlv {
    struct bgp_tlv*	next;
    uint8_t		type;
    uint8_t		len;
    void*		value;
};

struct pending {
    enum bgp_event_e	event;
    time_t		time;
    int			signal;
    uint8_t		override;
};


struct bgp_peer {
    uchar_t		version;
    uint16_t		as;
    uint32_t		id;
    uint16_t		port;
    uint16_t		hold_time;
    uint16_t		retry_time;
    uint16_t		keepalive_time;
    uint16_t		watchdog_time;
    char*		hostname;
    uint32_t		addr;
    uint8_t		options_size;
    struct bgp_tlv*	options;
    time_t		hold_expire;
    time_t		retry_expire;
    time_t		watchdog_expire;
    time_t		keepalive_expire;
    uint8_t		error;
    uint8_t		sub_error;
    uint64_t		info_updates;
    uint64_t		info_withdrawns;
    time_t		info_starttime;
    uint32_t		seq_idle_count;		  /* sequential idle count */
    enum bgp_flood_e	flood;			  /* boolean indicator during initial flood */
};

struct bgp_msg {
    uchar_t		raw[BGP_READBUF_MAX];
    uchar_t*		p;
    uint8_t		type;
    uint16_t		size;
};

/*
** The centre of the universe.
*/
struct bgp_session {
    struct bgp_peer	local;			  /* local peer information */
    struct bgp_peer	remote;			  /* remote peer information */
    enum bgp_state_e	state;			  /* current state of Finite State Machine */
    enum bgp_event_e	event;			  /* current event queued for Finite State Machine */
    int			sfd;			  /* socket file descriptor */
    struct sockaddr_in	saddr;			  /* socket structure */
    struct bgp_msg	msg;			  /* current queued or partially constructed BGP message */
    struct bgp_attr	attr;			  /* a store for transient BGP attribute information */
};

/*
** --------------------------------------------
*/
static int			bgp_send_message(struct bgp_session*);
static int			bgp_receive_message(struct bgp_session*);
static struct bgp_session*	bgp_create_session(char*);
static int			bgp_initiate_connection(struct bgp_session*);
static int			bgp_close_connection(struct bgp_session*);
static struct bgp_route*	bgp_decode_nlri(struct bgp_session*, uchar_t*, uint32_t);
static struct bgp_route*	bgp_decode_wd(struct bgp_session*, uchar_t*, uint32_t);
static int			bgp_decode_pa(struct bgp_session*, uchar_t*, uint32_t);
static int			bgp_decode_origin(struct bgp_session*, uchar_t*, uint32_t);
static int			bgp_decode_multi_exit_disc(struct bgp_session*, uchar_t*, uint32_t);
static int			bgp_decode_communities(struct bgp_session*, uchar_t*, uint32_t);
static int			bgp_decode_aspath(struct bgp_session*, uchar_t*, uint32_t);
static int			bgp_decode_nexthop(struct bgp_session*, uchar_t*, uint32_t);
static int			bgp_decode_local_pref(struct bgp_session*, uchar_t*, uint32_t);
static int			bgp_decode_aggregator(struct bgp_session*, uchar_t*, uint32_t);
static void			bgp_decode_message(struct bgp_session*);
static void			bgp_encode_message(struct bgp_session*, enum bgp_msg_type_e);
static void			bgp_restart_retry_timer(struct bgp_session*);
static void			bgp_restart_keepalive_timer(struct bgp_session*);
static void			bgp_restart_hold_timer(struct bgp_session*);
static void			bgp_restart_watchdog_timer(struct bgp_session*);
static void			bgp_signal_init(void);
static void			bgp_signal_handler(int, siginfo_t*, void*);
static void			bgp_exit(void);

static __inline__ size_t	bgp_set_uint8(uchar_t*, uint8_t);
static __inline__ size_t	bgp_set_uint16(uchar_t*, uint16_t);
static __inline__ size_t	bgp_set_uint32(uchar_t*, uint32_t);
static __inline__ size_t	bgp_get_uint8(uchar_t*, uint8_t*);
static __inline__ size_t	bgp_get_uint16(uchar_t*, uint16_t*);
static __inline__ size_t	bgp_get_uint32(uchar_t*, uint32_t*);

static void			bgp_print_message(uchar_t*, uint32_t);
static void			bgp_print_state(struct bgp_session*);
static void			bgp_print_usage(void);
static void			bgp_print_debug_settings(uint32_t);
static uint32_t			bgp_parse_debug_string(const char*, uint32_t);


static const char		filename[PATH_MAX];
static struct pending		pending;
static int			last_signal;

#define DEFAULT_FILENAME	"/var/tmp/bgp.log"

/*
**  The state machine in maintained in main() for the moment.
**
**  The session structure is designed to handle most of the state
**  information, so theoretically it wouldn't be difficult to modify 
**  for this this code to support multiple sessions (multiple peers).
**
*/
int main(int argc, char** argv){
    
    const char*		option_debug_str = "default,error,warning,info";
    uint16_t		option_port = 0;
    uint32_t		option_local_holdtime = BGP_DEFAULT_TIMER_HOLDTIME;
    uint32_t		option_local_watchdogtime = BGP_DEFAULT_TIMER_WATCHDOG;
    char*		option_remote_hostname = 0;
    uint32_t		option_local_addr = 0;
    char*		option_local_hostname = (char*) 0;
    uint16_t		option_local_as = 0;
    uint32_t		option_local_retry_time = BGP_DEFAULT_TIMER_CONNECT_RETRY;
    const char*		option_debug_filename = DEFAULT_FILENAME;
    
    struct bgp_session*	session;
    struct timeval	tv;
    char		local_hostname[64];
    
    int			c;
    extern char*	optarg;
    extern int		optind;
    int			errflg = 0;
    
    bgp_signal_init();
    
    while ((c = getopt(argc, argv, "o:d:vp:a:l:t:")) != EOF)
	switch (c) {
	case 'a':
	    option_local_as = (uint16_t) atoi(optarg);
	    break;
	case 'p':
	    option_port = (uint16_t) atoi(optarg);
	    break;
	case 'd':
	    option_debug_str = optarg;
	    break;
	case 't':
	    option_local_holdtime = atoi(optarg);
	    break;
	case 'l':
	    option_local_hostname = strdup(optarg);
	    break;
	case 'o':
	    option_debug_filename = strdup(optarg);
	    break;
	case '?':
	    errflg++;
	}
    
    
    if (optind != (argc - 1)) {
	errflg++;
	option_remote_hostname = NULL;
    } else {
	option_remote_hostname = strdup(argv[optind]);
    }
    
    if(option_debug_filename == NULL) {
	option_debug_filename = DEFAULT_FILENAME;
    }
    
    bgp_debug_init(option_debug_filename);
    
    /*
    ** OK - we can use debugs past here..
    **
    */ 

    /*
    ** calculate debug.
    */
    bgp_debug_level = BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_INFO;
    bgp_debug_level = bgp_parse_debug_string(option_debug_str, bgp_debug_level);
    bgp_print_debug_settings(bgp_debug_level);
    if(bgp_debug_level == 0) {
	bgp_print_usage();
	exit(1);
    }
    
    if ((!option_remote_hostname) || 
	(!option_local_as)) errflg++;
    if (errflg) {
	bgp_print_usage();
	exit(1);
    }
    if(!option_local_hostname) {
	gethostname(local_hostname, sizeof(local_hostname));
	option_local_hostname = local_hostname;
    }
    option_local_addr = ip_gethostbyname(option_local_hostname);
    bgp_debug(BGP_DEBUG_GENERAL, "using local hostname %s (%s)\n", option_local_hostname, ip_addr2str(option_local_addr));
    
    bgp_debug(BGP_DEBUG_INFO | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "\n");
    bgp_debug(BGP_DEBUG_INFO | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "bgp process started, pid=%d\n", getpid());
    
    /*
    ** create a bgp.pid file.
    */
    { 
	FILE*	pidfile;
	pidfile = fopen(BGP_PID_FILE, "w+");
	if(pidfile) {
	    fprintf(pidfile, "%d\n", (unsigned int) getpid());
	} else {
	    bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "WARNING - failed to write pid file - %s\n", BGP_PID_FILE);
	}
	fclose(pidfile);
    }
    
    /*
    ** create a new session.
    */
    if((session = bgp_create_session(option_remote_hostname))) {
	session->local.hostname = option_local_hostname;
	session->local.addr = option_local_addr;
	session->local.version = bgp_VERSION_4;
	session->local.as = option_local_as;
	session->local.hold_time = option_local_holdtime;
	session->local.id = option_local_addr;
	session->local.retry_time = option_local_retry_time;
	session->local.watchdog_time = option_local_watchdogtime;
	session->remote.port = BGP_DEFAULT_PORT;
    } else {
	bgp_debug(BGP_DEBUG_GENERAL, "session startup failed\n");
	exit(1);		/* XXX - euw. */
    }
    
    /*
    ** initialise the magic CIDR database..  This is imported from the
    ** ip2cidr library, which maintains a shared memory segment 
    ** and presents an API to store routes and opaque data.
    */ 
    mpm_debug_init(argv[0]);
    if(cidr_init("bgp",0) != 0) {
	bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "ERROR - failed to initialise cidr database\n");
	exit(1);
    }
    
    /*
    ** Finite State Machine.. 
    ** 
    ** See RFC1771 for the rules.
    */
    
    session->event = bgp_EVENT_START;
    session->state = bgp_STATE_IDLE;
    session->local.retry_expire = 0;
    session->local.watchdog_expire = 0;
    session->local.hold_expire = 0;
    session->local.keepalive_expire = 0;
    
    pending.event = bgp_EVENT_IDLE;
    pending.time = 0;
    pending.signal = 0;
    pending.override = 1; /* override any other events, since pending is used to terminate connections */
    
    bgp_restart_watchdog_timer(session);
    
    while(1) {
	uint32_t	old_state;
	char*		timestr;
	
	/*
	** print a nice state header if required.
	*/
	gettimeofday(&tv, NULL);
	timestr = ctime(&tv.tv_sec);
	timestr[24] = '\0';
	bgp_debug(BGP_DEBUG_TIMERS, "-----------\n");
	bgp_debug(BGP_DEBUG_TIMERS, "STATE: %s(%d)\n", bgp_state_names[session->state], session->state);
	bgp_debug(BGP_DEBUG_TIMERS, "EVENT: %s(%d)\n", bgp_event_names[session->event], session->event);
	if(pending.event != bgp_EVENT_IDLE) {
	    bgp_debug(BGP_DEBUG_TIMERS, "PENDING EVENT: %s(%d)\n", bgp_event_names[pending.event], pending.event);
	}
	bgp_debug(BGP_DEBUG_TIMERS, "TIME: %s (%d)\n", timestr, (int) tv.tv_sec);
	bgp_debug(BGP_DEBUG_TIMERS, "ACTIVE TIMERS: ");
	if(session->local.retry_expire) 
	    bgp_debug(BGP_DEBUG_TIMERS, "retry_expire=%d(%d) ", 
		      (int) session->local.retry_expire,
		      (int) session->local.retry_expire - (int) tv.tv_sec);
	if(session->local.keepalive_expire)
	    bgp_debug(BGP_DEBUG_TIMERS, "keepalive_expire=%d(%d) ",
		      (int) session->local.keepalive_expire,
		      (int) session->local.keepalive_expire - (int) tv.tv_sec);
	if(session->local.hold_expire)
	    bgp_debug(BGP_DEBUG_TIMERS, "hold_expire=%d(%d) ",
		      (int) session->local.hold_expire,
		      (int) session->local.hold_expire - (int) tv.tv_sec);
	if(session->local.watchdog_expire)
	    bgp_debug(BGP_DEBUG_TIMERS, "watchdog_expire=%d(%d) ",
		      (int) session->local.watchdog_expire,
		      (int) session->local.watchdog_expire - (int) tv.tv_sec);
	bgp_debug(BGP_DEBUG_TIMERS, "\n");
	
	if(session->event == bgp_EVENT_IDLE) {
	    /*
	    ** check for end of the flood of updates..
	    ** This is simply detecting a number of sequential idle 
	    ** events after at least one routing update has been seen.
	    */
	    if(session->state == bgp_STATE_ESTABLISHED && session->remote.flood == bgp_FLOOD_STARTED) {
		session->remote.seq_idle_count++;
		if((session->remote.flood) && (session->remote.seq_idle_count > BGP_FLOOD_END_IDLE_COUNT)) {
		    time_t	nowtime;
		    time_t	deltatime;
		    int		purge_counter;
		    
		    nowtime = time((time_t*) NULL);
		    deltatime = nowtime - session->remote.info_starttime;
		    session->remote.flood = bgp_FLOOD_COMPLETED;
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "flood completed from %s - %llu routing updates and %llu widthrawn routes in %d seconds\n",
			      session->remote.hostname, session->remote.info_updates, session->remote.info_withdrawns, deltatime);
		    purge_counter = cidr_purge(bgp_decrement_reference);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "purged %d expired networks\n", purge_counter);
		}
	    }
	} else {
	    /*
	    ** check for flood start.
	    */
	    session->remote.seq_idle_count = 0;
	    if(session->state == bgp_STATE_ESTABLISHED && session->remote.info_updates > 1 && session->remote.flood == bgp_FLOOD_NOT_STARTED) {
		session->remote.flood = bgp_FLOOD_STARTED;
		bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			  "flood has commenced\n");
	    }
	}
		
	/*
	** Terminate, if required.
	*/
	if(session->event == bgp_EVENT_TERMINATE) {
	    if(session->state == bgp_STATE_ESTABLISHED)
	    bgp_close_connection(session);
	    exit(1);
	}
	
	/*
	** The "IDLE" event was added so we can support polling.
	*/
	if(session->event != bgp_EVENT_IDLE) {
	    old_state = session->state;
	    switch ((int) session->state) {
		
		/*
		** IDLE - start the connection.
		**
		*/
	    case bgp_STATE_IDLE:
		switch((int) session->event) {
		case bgp_EVENT_START:
		    
		    /*
		    ** give the router some thinking time, so hopefully we
		    ** don't get a connection refused on fast re-connects
		    */
		    sleep(2);
		    
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, "   -- restarting FSM from start event\n");
		    
		    session->event = bgp_EVENT_IDLE;
		    session->remote.flood = bgp_FLOOD_NOT_STARTED;
		    session->local.error = 0;
		    session->local.sub_error = 0;
		    bgp_initiate_connection(session);
		    bgp_restart_retry_timer(session);
		    session->state = bgp_STATE_CONNECT;
		    {
			int expire_counter;
			expire_counter = cidr_expireall();
			bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP,
				  "expired %d existing networks for database\n", expire_counter);
		    }
		    break;
		    
		case bgp_EVENT_TERMINATE:
		    bgp_exit();
		    break;
		    
		case bgp_EVENT_STOP:
		    session->event = bgp_EVENT_START;
		    session->state = bgp_STATE_IDLE;
		    break;
		    
		default:
		    session->local.hold_expire = 0;
		    session->local.keepalive_expire = 0;
		    session->local.retry_expire = 0;
		    session->state = bgp_STATE_IDLE;
		    session->event = bgp_EVENT_START; /* do the thing again. */
		    break;
		}
		break;
		
		/*
		** CONNECT - 
		** 
		*/
	    case bgp_STATE_CONNECT:
		switch((int) session->event) {
		    
		case bgp_EVENT_START:
		    session->event = bgp_EVENT_IDLE;
		    break;
		
		case bgp_EVENT_TERMINATE:
		    bgp_exit();
		    break;
		    
		case bgp_EVENT_STOP:
		    if(session->local.error) {
			bgp_encode_message(session, bgp_MSG_NOTIFICATION);
			bgp_send_message(session);
		    }
		    session->event = bgp_EVENT_START;
		    session->state = bgp_STATE_IDLE;
		    break;
		    
		case bgp_EVENT_TRANSPORT_OPEN:
		    session->event = bgp_EVENT_IDLE;
		    bgp_encode_message(session, bgp_MSG_OPEN);
		    bgp_send_message(session);
		    session->local.retry_expire = 0;
		    session->state = bgp_STATE_OPENSENT;
		    break;
		    
		case bgp_EVENT_TRANSPORT_OPEN_FAILED:
		    session->event = bgp_EVENT_IDLE;
		    bgp_restart_retry_timer(session);
		    bgp_initiate_connection(session);
		    session->state = bgp_STATE_ACTIVE;
		    break;
		    
		case bgp_EVENT_RETRY_TIMER_EXPIRE:
		    session->event = bgp_EVENT_IDLE;
		    bgp_restart_retry_timer(session);
		    bgp_initiate_connection(session);
		    session->state = bgp_STATE_CONNECT;
		    break;
		    
		default:
		    session->event = bgp_EVENT_IDLE;
		    session->state = bgp_STATE_IDLE;
		    break;
		}
		break;
		
		
		/*
		** ACTIVE - we should send an OPEN message.
		**          and move to state OPENSENT.
		*/
	    case bgp_STATE_ACTIVE:					
		switch((int) session->event) {
		    
		case bgp_EVENT_START:
		    session->event = bgp_EVENT_IDLE;
		    break;
		    
		case bgp_EVENT_TERMINATE:
		    bgp_exit();
		    break;
		    
		case bgp_EVENT_STOP:
		    if(session->local.error) {
			bgp_encode_message(session, bgp_MSG_NOTIFICATION);
			bgp_send_message(session);
		    }
		    session->event = bgp_EVENT_START;
		    session->state = bgp_STATE_IDLE;
		    break;

		case bgp_EVENT_TRANSPORT_OPEN:
		    session->event = bgp_EVENT_IDLE;
		    bgp_encode_message(session, bgp_MSG_OPEN);
		    bgp_send_message(session);
		    session->local.retry_expire = 0;
		    session->state = bgp_STATE_OPENSENT;
		    break;
		    
		case bgp_EVENT_TRANSPORT_OPEN_FAILED:
		    session->event = bgp_EVENT_IDLE;
		    bgp_close_connection(session);
		    bgp_restart_retry_timer(session);
		    session->state = bgp_STATE_ACTIVE;
		    break;
		    
		case bgp_EVENT_RETRY_TIMER_EXPIRE:
		    session->event = bgp_EVENT_IDLE;
		    bgp_restart_retry_timer(session);
		    bgp_initiate_connection(session);
		    session->state = bgp_STATE_CONNECT;
		    break;
		    
		default:
		    session->event = bgp_EVENT_IDLE;
		    session->state = bgp_STATE_IDLE;
		    break;
		}
		
		break;
		
		/*
		** OPENSENT - wait for an OPEN to arrive from the 
		**            remote peer.
		**
		*/
	    case bgp_STATE_OPENSENT:
		switch((int) session->event) {
		    
		case bgp_EVENT_START:
		    session->event = bgp_EVENT_IDLE;
		    break;
		    
		case bgp_EVENT_TERMINATE:
		    bgp_close_connection(session);
		    bgp_exit();
		    break;
		    
		case bgp_EVENT_STOP:
		    if(session->local.error) {
			bgp_encode_message(session, bgp_MSG_NOTIFICATION);
			bgp_send_message(session);
		    }
		    bgp_close_connection(session);
		    session->event = bgp_EVENT_START;
		    session->state = bgp_STATE_IDLE;
		    break;

		case bgp_EVENT_TRANSPORT_CLOSED:
		    session->event = bgp_EVENT_IDLE;
		    bgp_close_connection(session);
		    bgp_restart_retry_timer(session);
		    break;
		    
		case bgp_EVENT_TRANSPORT_FATAL:
		    session->event = bgp_EVENT_IDLE;
		    session->state = bgp_STATE_IDLE;
		    break;
		    
		case bgp_EVENT_RX_OPEN:
		    session->event = bgp_EVENT_IDLE;
		    bgp_decode_message(session);
		    session->local.keepalive_time = session->remote.hold_time / 3;
		    bgp_encode_message(session, bgp_MSG_KEEPALIVE);
		    bgp_restart_keepalive_timer(session);
		    bgp_send_message(session);
		    session->state = bgp_STATE_OPENCONFIRM;
		    
		    bgp_debug(BGP_DEBUG_INFO | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, 
			      "negotiating: ");
		    bgp_debug(BGP_DEBUG_INFO | BGP_DEBUG_STDOUT,
			      "%s(%s) AS%d -> ",
			      session->local.hostname,
			      ip_addr2str(session->local.addr),
			      session->local.as);
		    bgp_debug(BGP_DEBUG_INFO | BGP_DEBUG_STDOUT,
			      "%s(%s,%d) AS%d\n",
			      session->remote.hostname,
			      ip_addr2str(session->remote.addr),
			      session->remote.port,
			      session->remote.as);
		    
		    break;
		    
		    /* XXX what if a timer expires here */
		default:
		    session->event = bgp_EVENT_IDLE;
		    bgp_close_connection(session);
		    session->state = bgp_STATE_IDLE;
		}
		break;
		
		/*
		** OPENCONFIRM - wait for a keepalive and move to ESTABLISHED.
		**
		*/
	    case bgp_STATE_OPENCONFIRM:
		switch((int) session->event) {
		    
		case bgp_EVENT_START:
		    session->event = bgp_EVENT_IDLE;
		    break;

		case bgp_EVENT_TERMINATE:
		    bgp_close_connection(session);
		    bgp_exit();
		    break;
		    
		case bgp_EVENT_STOP:
		    if(session->local.error) {
			bgp_encode_message(session, bgp_MSG_NOTIFICATION);
			bgp_send_message(session);
		    }
		    bgp_close_connection(session);
		    session->event = bgp_EVENT_START;
		    session->state = bgp_STATE_IDLE;
		    break;

		case bgp_EVENT_TRANSPORT_CLOSED:
		    bgp_close_connection(session);
		    session->event = bgp_EVENT_START;
		    session->state = bgp_STATE_IDLE;
		    break;
		    
		case bgp_EVENT_TRANSPORT_FATAL:
		    bgp_close_connection(session);
		    session->event = bgp_EVENT_IDLE;
		    session->state = bgp_STATE_IDLE;
		    break;
		    
		case bgp_EVENT_KEEPALIVE_TIMER_EXPIRE:
		    bgp_restart_keepalive_timer(session);
		    bgp_encode_message(session, bgp_MSG_KEEPALIVE);
		    bgp_send_message(session);
		    session->event = bgp_EVENT_IDLE;
		    session->state = bgp_STATE_OPENCONFIRM;
		    break;
		    
		case bgp_EVENT_RX_KEEPALIVE:
		    bgp_decode_message(session);
		    bgp_restart_hold_timer(session);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "bgp connection established to %s\n", session->remote.hostname);
		    session->event = bgp_EVENT_IDLE;
		    session->state = bgp_STATE_ESTABLISHED;
		    break;
		    
		case bgp_EVENT_RX_NOTIFICATION:
		    bgp_close_connection(session);
		    session->event = bgp_EVENT_IDLE;
		    session->state = bgp_STATE_IDLE;
		    break;
		    
		default:
		    session->local.error = 1;
		    session->local.sub_error = 1;
		    bgp_encode_message(session, bgp_MSG_NOTIFICATION);
		    bgp_send_message(session);
		    bgp_close_connection(session);
		    session->state = bgp_STATE_IDLE;
		    session->event = bgp_EVENT_START;
		    break;
		}
		break;
		
		
		/*
		** This is where we spend most of our life.
		*/
	    case bgp_STATE_ESTABLISHED: 
		switch((int) session->event) {
		    
		case bgp_EVENT_START:
		    session->event = bgp_EVENT_IDLE;
		    break;
		    
		case bgp_EVENT_TERMINATE:
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "Terminate event - disconnecting BGP session to %s\n", session->remote.hostname);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "session info - %llu routing updates and %llu widthrawn routes\n",
			      session->remote.info_updates, session->remote.info_withdrawns);
		    bgp_close_connection(session);
		    bgp_exit();
		    break;
		    
		case bgp_EVENT_STOP:
		    if(session->local.error) {
			bgp_encode_message(session, bgp_MSG_NOTIFICATION);
			bgp_send_message(session);
		    }
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "Stop event - disconnecting BGP session to %s\n", session->remote.hostname);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "session info - %llu routing updates and %llu widthrawn routes\n",
			      session->remote.info_updates, session->remote.info_withdrawns);
		    bgp_close_connection(session);
		    session->event = bgp_EVENT_START;
		    session->state = bgp_STATE_IDLE;
		    break;

		case bgp_EVENT_TRANSPORT_FATAL:
		case bgp_EVENT_TRANSPORT_CLOSED:
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "Transport Closed event - closing down BGP session to %s\n", session->remote.hostname);
		    bgp_close_connection(session);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "session info - %llu routing updates and %llu widthrawn routes\n",
			      session->remote.info_updates, session->remote.info_withdrawns);
		    session->event = bgp_EVENT_START;
		    session->state = bgp_STATE_IDLE;
		    break;
		    
		case bgp_EVENT_KEEPALIVE_TIMER_EXPIRE:
		    session->event = bgp_EVENT_IDLE;
		    bgp_restart_keepalive_timer(session);
		    bgp_encode_message(session, bgp_MSG_KEEPALIVE);
		    bgp_send_message(session);
		    break;
		    
		case bgp_EVENT_RX_KEEPALIVE:
		    session->event = bgp_EVENT_IDLE;
		    bgp_decode_message(session);
		    bgp_restart_hold_timer(session);
		    break;
		    
		case bgp_EVENT_RX_UPDATE:
		    session->event = bgp_EVENT_IDLE;
		    bgp_decode_message(session);
		    bgp_restart_hold_timer(session);
		    break;
		    
		case bgp_EVENT_RX_NOTIFICATION:
		    session->event = bgp_EVENT_IDLE;
		    bgp_decode_message(session);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "Received a Notification message of an error from remote host: type %d, subtype %d\n",
			      session->local.error, session->local.sub_error);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "session info - %llu routing updates and %llu widthrawn routes\n",
			      session->remote.info_updates, session->remote.info_withdrawns);
		    session->event = bgp_EVENT_START;
		    session->state = bgp_STATE_IDLE;
		    break;
		    
		case bgp_EVENT_HOLD_TIMER_EXPIRE:
		    session->event = bgp_EVENT_IDLE;
		    session->local.error = bgp_NOTIFY_HOLD_TIMER;
		    session->local.sub_error = 0;
		    bgp_encode_message(session, bgp_MSG_NOTIFICATION);
		    bgp_send_message(session);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
			      "WARNING - HOLD TIMER expired, sending notification and closing connection\n");
		    bgp_close_connection(session);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "session info - %llu routing updates and %llu widthrawn routes\n",
			      session->remote.info_updates, session->remote.info_withdrawns);
		    session->state = bgp_STATE_IDLE;
		    session->event = bgp_EVENT_START;
		    break;
		    
		default:
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
			      "A state machine error has occured - received event %d in state ESTABLISHED\n", session->event);
		    
		    session->event = bgp_EVENT_IDLE;
		    session->local.error = bgp_NOTIFY_FSM;
		    session->local.sub_error = 0;
		    bgp_encode_message(session, bgp_MSG_NOTIFICATION);
		    bgp_send_message(session);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
			      "sending notification and closing connection\n");
		    bgp_close_connection(session);
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
			      "session info - %llu routing updates and %llu widthrawn routes\n",
			      session->remote.info_updates, session->remote.info_withdrawns);
		    session->state = bgp_STATE_IDLE;
		    session->event = bgp_EVENT_START;
		    
		    break;
		}
		break;
		
	    default:
		bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
			  "state machine in unknown state %d - closing connection\n", (int) old_state);
		session->local.error = bgp_NOTIFY_FSM;
		session->local.sub_error = 0;
		bgp_encode_message(session, bgp_MSG_NOTIFICATION);
		bgp_send_message(session);
		bgp_close_connection(session);
		session->state = bgp_STATE_IDLE;
		session->event = bgp_EVENT_START;
		
		break;
	    }
	}
	
	/*
	** Timer Housekeeping.
	**
	*/ 
	gettimeofday(&tv, NULL);
	if (session->event == bgp_EVENT_IDLE && session->local.keepalive_expire) {
	    if(tv.tv_sec > session->local.keepalive_expire)
		session->event = bgp_EVENT_KEEPALIVE_TIMER_EXPIRE;
	}
	if (session->event == bgp_EVENT_IDLE && session->local.hold_expire) {
	    if(tv.tv_sec > session->local.hold_expire) 
		session->event = bgp_EVENT_HOLD_TIMER_EXPIRE;
	}
	if (session->event == bgp_EVENT_IDLE && session->local.retry_expire) {
	    if(tv.tv_sec > session->local.retry_expire)
		session->event = bgp_EVENT_RETRY_TIMER_EXPIRE;
	}
	
	/*
	** watchdog timers don't generate events - they're handled on the spot 
	** and do not effect the state machine at all.
	*/ 
	if (session->local.watchdog_expire) {
	    if(tv.tv_sec > session->local.watchdog_expire) {
		if (session->state == bgp_STATE_ESTABLISHED) {
		    cidr_watchdog_tick();
		    bgp_restart_watchdog_timer(session);
		}
	    }
	}
	
	/*
	** pending is used for terminating async events, such as
	** signals.
	**
	** XXX - this totally destorys the current real
	** event, so pending events can only be used for
	** async terminates and stops.
	*/
	if(pending.signal != 0) {
	    char	sigbuf[SIG2STR_MAX];
	    char*	ts;
	    ts = ctime(&pending.time);
	    ts[24] = '\0';
	    sig2str(pending.signal, sigbuf);
	    
	    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_WARNING | BGP_DEBUG_TIMESTAMP,
		      "SIG%s(%d) received at %s, pending %s event created\n",
		      sigbuf, pending.signal, ts, bgp_event_names[pending.event]);
	    pending.signal = 0;
	}
	
	/*
	** handle pending events..
	** 
	** if override is enabled, the current event will be trashed,
	** most likely clagging the FSM.. This is OK, since all pending
	** events result in the restart of the FSM anyway.
	*/
	if(pending.event != bgp_EVENT_IDLE) {
	    if(((pending.override == 0) && (session->event == bgp_EVENT_IDLE)) || (pending.override == 1)) {
		if((pending.override == 1) && (session->event != bgp_EVENT_IDLE)) {
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_WARNING | BGP_DEBUG_TIMESTAMP, "WARNING - event %s(%d) was discarded to handle pending event\n", bgp_event_names[session->event], session->event);
		}
		if((pending.override == 0)) {
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_WARNING | BGP_DEBUG_TIMESTAMP, "queuing pending event %s(%d) until idle\n", bgp_event_names[session->event], session->event);
		}
		
		session->event = pending.event;
		pending.event = bgp_EVENT_IDLE;
	    }
	}
	
	/*
	** Look for incoming messages,
	** only if onthing else interesting is happening
	*/
	if(session->event == bgp_EVENT_IDLE) {
	    if(session->state == bgp_STATE_ESTABLISHED ||
	       session->state == bgp_STATE_OPENSENT ||
	       session->state == bgp_STATE_OPENCONFIRM) {
		bgp_receive_message(session);
	    }
	}
	
	/*
	** Last of all, sleep around if we're really idle.
	*/ 
	if(session->event == bgp_EVENT_IDLE) {
	    sleep(BGP_POLL_LOOP_IDLE_SLEEP_TIME);
	}
	
	/*	fflush(stdout); */
    }
    return 0;
}

static void	bgp_restart_watchdog_timer(struct bgp_session* session) {
    struct timeval	tv;
    
    gettimeofday(&tv, NULL);
    bgp_debug(BGP_DEBUG_EXPIRE | BGP_DEBUG_TIMESTAMP , "watchdog_expire=%d(%d)\n",
	      (int) session->local.watchdog_expire,
	      (int) session->local.watchdog_expire - (int) tv.tv_sec);
    
    session->local.watchdog_expire = tv.tv_sec + session->local.watchdog_time;
    return;
}

static void	bgp_restart_retry_timer(struct bgp_session* session) {
    struct timeval	tv;
    
    gettimeofday(&tv, NULL);
    bgp_debug(BGP_DEBUG_EXPIRE | BGP_DEBUG_TIMESTAMP, "retry_expire=%d(%d)\n", 
	      (int) session->local.retry_expire,
	      (int) session->local.retry_expire - (int) tv.tv_sec);
    
    session->local.retry_expire = tv.tv_sec + session->local.retry_time;
    return;
}

static void	bgp_restart_keepalive_timer(struct bgp_session* session) {
    struct timeval	tv;
    
    gettimeofday(&tv, NULL);
    bgp_debug(BGP_DEBUG_EXPIRE | BGP_DEBUG_TIMESTAMP, "keepalive_expire=%d(%d)\n",
	      (int) session->local.keepalive_expire,
	      (int) session->local.keepalive_expire - (int) tv.tv_sec);
    session->local.keepalive_expire = tv.tv_sec + session->local.keepalive_time;
    return;
}

static void	bgp_restart_hold_timer(struct bgp_session* session) {
    struct timeval	tv;
    
    gettimeofday(&tv, NULL);
    bgp_debug(BGP_DEBUG_EXPIRE | BGP_DEBUG_TIMESTAMP, "hold_expire=%d(%d)\n",
	      (int) session->local.hold_expire,
	      (int) session->local.hold_expire - (int) tv.tv_sec);
    session->local.hold_expire = tv.tv_sec + session->local.hold_time;
    return;
}
    
/*
**
**
*/
static int bgp_send_message(struct bgp_session* session) {
    
    int ret;
    struct bgp_msg* msg;
    
    msg = &session->msg;
    
    bgp_debug(BGP_DEBUG_PACKET, "bgp_send_message(%s)\n", bgp_msg_type_names[msg->type]);
    if (msg->size != msg->p - msg->raw) {
	bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP,
		  "internal packet size mismatch before send, %d != %d\n", msg->size, msg->p - msg->raw);
	bgp_close_connection(session);
	session->event = bgp_EVENT_STOP;
	return 1;
    }
    
    ret = write(session->sfd, msg->raw, msg->size);
    if (ret != msg->size) {
	/* XXX - should return an event */
	return -1;
    }
    return 0;
}

static int bgp_close_connection(struct bgp_session* session) {
    
    close(session->sfd);
    
    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
	      "closing connection to %s\n", session->remote.hostname);
    /* XXX - free any memory? */
    return 0;
    
}

/*
**
**
*/
static int bgp_initiate_connection(struct bgp_session* session) {
    
    int		ret;
    int		val;
    
    /*
    ** open the socket..
    */
    
    bgp_debug(BGP_DEBUG_INFO | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, 
	      "opening socket to %s (%s) on port %d\n",
	      session->remote.hostname, ip_addr2str(session->remote.id), session->remote.port);
    
    session->remote.info_starttime = time((time_t*) NULL);
    session->remote.info_withdrawns = 0;
    session->remote.info_updates = 0;
    session->remote.flood = bgp_FLOOD_NOT_STARTED;
    
    session->local.keepalive_expire = 0;
    session->local.hold_expire = 0;
    
    session->sfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&session->saddr, sizeof(session->saddr));
    session->saddr.sin_family = AF_INET;
    session->saddr.sin_addr.s_addr = htonl(session->remote.id);
    session->saddr.sin_port = htons(session->remote.port);
    
    ret = connect(session->sfd, (struct sockaddr *) &session->saddr, sizeof(session->saddr));
    if (ret < 0) {
	bgp_close_connection(session);
	session->event = bgp_EVENT_TRANSPORT_OPEN_FAILED;
	bgp_debug(BGP_DEBUG_INFO | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, 
		  "connect() to host %s failed - returned %s\n", session->remote.hostname, strerror(errno));
	return -1;
    }
    
    val = fcntl(session->sfd, F_GETFL, 0);
    fcntl(session->sfd, F_SETFL, val | O_NONBLOCK);
    session->event = bgp_EVENT_TRANSPORT_OPEN;
    return 0;
}    


/*
** read an incoming message.
**
*/
static int bgp_receive_message(struct bgp_session* session) {
    
    struct bgp_msg*	msg;
    int			ret;
    int			finished;
    int			bytes_to_read;
    uint32_t		marker[4];
    
    msg = &session->msg;
    msg->p = msg->raw;
    msg->size = 0;
    msg->type = 0;
    
    /*
    ** fetch the header, so we know the size of the rest of the message..
    **
    ** As usual, read() can return any amount of bytes it likes, so we must keep 
    ** looping until we receive the correct amount.  This may mean that we continuously
    ** loop and sleep inside this function, if something is wrong, we may eventually
    ** miss the send of a keepalive. (120 seconds of waiting here).
    **
    ** This scenario can't really happen, unless the other end is screwed totally. (Since
    ** its supposed to send keepalives too, which enter this read queue.)  Either way
    ** the connection will be dropped (remotely) and we will eventually leave this function.
    **
    ** We read the BGP4 header first.. Within the header will be a length field, which will
    ** let us calculate the size of the body read.
    */ 
    
    bytes_to_read = BGP_HEADER_LENGTH;
    while(bytes_to_read) {
	bgp_debug(BGP_DEBUG_VERBOSE, "reading header, %d bytes left of %d\n", bytes_to_read, BGP_HEADER_LENGTH);
	ret = read(session->sfd, msg->p, bytes_to_read);
	
	/*
	** error conditions.
	*/
	if (ret < 0) {
	    switch(errno) {
		/*
		** O_NONBLOCK read() on fd with no data to read.
		*/
	    case EAGAIN:
		if(bytes_to_read == BGP_HEADER_LENGTH) {
		    /*
		    ** there was nothing to read, and we haven't read a byte yet,
		    ** so lets go back to the FSM.
		    */
		    session->event = bgp_EVENT_IDLE;
		    return(-1);
		} else {
		    /*
		    ** We've already read something, so lets stay here until
		    ** its all done.
		    */
		    sleep(1);		       
		}
		break;
		
	    case EINTR:
		/*
		** I'm not sure if this are going to occur, lets catch it anyway, and continue
		** to read the data.
		*/
		bgp_debug(BGP_DEBUG_WARNING | BGP_DEBUG_TIMESTAMP, "interrupted system call on socket read on header - %s\n", strerror(errno));
		break;
		
	    default:
		bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
			  "unexpected errno during header rx - %s\n", strerror(errno));
		session->event = bgp_EVENT_STOP;
		return -1;
	    }
	} else if (ret == 0) {
	    /*
	    ** The connection has closed.
	    */
	    bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP | BGP_DEBUG_STDOUT,
		      "ERROR part of the header read returned 0, connection closed.\n");
	    session->event = bgp_EVENT_TRANSPORT_CLOSED;
	    return(-1);
	} else {
	    /*
	    ** otherwise, we read some data..
	    */
	    bytes_to_read -= ret;
	    msg->p += ret;
	}
    }
    
    /*
    ** decode the marker in the head, to ensure it actually looks like a header 
    */
    bgp_get_uint32(&msg->raw[0], &marker[0]);			/* marker */
    bgp_get_uint32(&msg->raw[4], &marker[1]);			/* marker */
    bgp_get_uint32(&msg->raw[8], &marker[2]);			/* marker */
    bgp_get_uint32(&msg->raw[12], &marker[3]);			/* marker */
    bgp_get_uint16(&msg->raw[16], &msg->size);
    bgp_get_uint8(&msg->raw[18], &msg->type);
    
    /* bgp_print_message(msg->raw, BGP_HEADER_LENGTH); */
    
    if (~marker[0] & ~marker[1] & ~marker[2] & ~marker[3]) {
	bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
		  "broken marker, stream may be out of sync\n");
	bgp_print_message(msg->raw, BGP_HEADER_LENGTH);
	session->local.error = bgp_NOTIFY_MSG_HEADER;
	session->local.sub_error = bgp_NOTIFY_HEADER_LOST_SYNC;
	session->event = bgp_EVENT_STOP;
	return (-1);
    }
    /* printf("messages received of size %d, type %d\n", msg->size, msg->type); */
    
    if(msg->size < BGP_HEADER_LENGTH) {
	bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
		  "runt packet - closing connection to resync\n");
	session->local.error = bgp_NOTIFY_MSG_HEADER;
	session->local.sub_error = bgp_NOTIFY_HEADER_BAD_LEN;
	session->event = bgp_EVENT_STOP;
	return(-1);
    }
    
    if(msg->size > BGP_READBUF_MAX - 1) {
	bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
		  "giant oversized packet will overflow read buffer - closing connection to resync\n");
	session->local.error = bgp_NOTIFY_MSG_HEADER;
	session->local.sub_error = bgp_NOTIFY_HEADER_BAD_LEN;
	session->event = bgp_EVENT_STOP;
	return(-1);
    }	
    
    /*
    ** fetch the rest of the packet.
    */
    finished = 0;
    if(msg->size > BGP_HEADER_LENGTH) {
	bytes_to_read = msg->size - BGP_HEADER_LENGTH;
	while (bytes_to_read) {
	    bgp_debug(BGP_DEBUG_VERBOSE, "reading body, %d bytes left of %d\n", bytes_to_read, msg->size - BGP_HEADER_LENGTH);
	    ret = read(session->sfd, msg->p, bytes_to_read);
	    if (ret < 0) {
		switch(errno) {
		case EAGAIN:
		    sleep(1);
		    break;
		case EINTR:
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_WARNING | BGP_DEBUG_TIMESTAMP, "in bgp read body - %s\n", strerror(errno));
		    break;
		default:
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
			      "unexpected error from read() - %s\n", strerror(errno));
		    session->local.error = bgp_NOTIFY_CEASE;
		    session->local.sub_error = 0;
		    session->event = bgp_EVENT_STOP;
		    return(-1);
		}
	    } else {
		/* printf("read %d bytes\n", ret); */
		if (ret == 0) {
		    bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_ERROR | BGP_DEBUG_TIMESTAMP, 
			      "transport closed from remote end\n");
		    session->event = bgp_EVENT_TRANSPORT_CLOSED;
		    return(-1);
		}
		msg->p += ret;
		bytes_to_read -= ret;
	    }
	}
    }
    
    /*
    ** reset the offset
    */
    msg->p = msg->raw;
    if(msg->type >= bgp_min_MSG && msg->type <= bgp_max_MSG) {
	bgp_debug(BGP_DEBUG_PACKET, "messages type %s(%d) size %d\n", bgp_msg_type_names[msg->type], msg->type, msg->size);
    } else {
	bgp_debug(BGP_DEBUG_PACKET, "messages type ERROR(%d) size %d\n", msg->type, msg->size);
	/* XXX - generate an event */
	return -1;
    }
    
    /*
    ** Let the FSM know what we found.
    */
    switch((int) msg->type) {
    case bgp_MSG_OPEN:
	session->event = bgp_EVENT_RX_OPEN;
	break;
    case bgp_MSG_KEEPALIVE:
	session->event = bgp_EVENT_RX_KEEPALIVE;
	break;
    case bgp_MSG_UPDATE:
	session->event = bgp_EVENT_RX_UPDATE;
	break;
    case bgp_MSG_NOTIFICATION:
	session->event = bgp_EVENT_RX_NOTIFICATION;
	break;
    default:
	session->event = bgp_EVENT_STOP;
	session->local.error = bgp_NOTIFY_MSG_HEADER;
	session->local.error = bgp_NOTIFY_HEADER_BAD_TYPE;
	return(-1);
    }
    return 0;
}

/*
**
**
*/
struct bgp_session* bgp_create_session(char* h) {
    struct bgp_session*	session;
    uint32_t		remote_addr;
    char*		remote_hostname;
    
    remote_hostname = strdup(h); /* never free()'d */
    remote_addr = ip_gethostbyname(remote_hostname);
    if(remote_addr == 0) {
	    bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP , "ERROR - failed to resolve remote hostname %s\n", remote_hostname);
	    exit(1);
    }
	    
    bgp_debug(BGP_DEBUG_GENERAL, "remote host is %s (%s)\n", remote_hostname, ip_addr2str(remote_addr));
    
    session = malloc(sizeof(struct bgp_session));
    bzero(session, sizeof(struct bgp_session));
    session->remote.id = remote_addr;
    session->remote.hostname = remote_hostname;
    session->remote.hold_expire = 0;
    session->remote.hold_time = 0;
    session->remote.keepalive_expire = 0;
    session->local.keepalive_expire = 0;
    session->local.hold_expire = 0;
    session->remote.addr = remote_addr;
    /* session->attr.communities = (uint32_t*) malloc(sizeof(uint32_t) * BGP_COMMUNITY_STORE_SIZE); */
    /* session->attr.aspath = (uint32_t*) malloc(sizeof(uint32_t) * BGP_ASPATH_STORE_SIZE); */
    
    return session;
}

/*
**
*/
int bgp_decode_multi_exit_disc(struct bgp_session* session, uchar_t* p, uint32_t length) {
    uint32_t	value;
    
    if (length != sizeof(uint32_t)) {
	bgp_debug(BGP_DEBUG_ERROR, "ERROR - MULTI_EXIT_DISC length was wrong -> %d\n", (unsigned int) length);
    } else {
	p += bgp_get_uint32(p, &value);
	bgp_debug(BGP_DEBUG_MSG, "0x%08X", (unsigned int) value);
	session->attr.multiexit = value;
    }
    
    return 0;
}

/*
**
*/
int bgp_decode_aggregator(struct bgp_session* session, uchar_t* p, uint32_t length) {
    
    if (length != 6) {
	bgp_debug(BGP_DEBUG_ERROR, "ERROR - AGGREGATOR length was wrong -> %d\n", (unsigned int) length);
    } else {
	p += bgp_get_uint16(p, &session->attr.aggregator_as);
	p += bgp_get_uint32(p, &session->attr.aggregator_ip);
	bgp_debug(BGP_DEBUG_MSG, "AS %d, %s", session->attr.aggregator_as, ip_addr2str(session->attr.aggregator_ip));
    }
    
    return 0;
}


/*
**
*/
int bgp_decode_local_pref(struct bgp_session* session, uchar_t* p, uint32_t length) {
    uint32_t value;
    
    if (length != 4) {
	bgp_debug(BGP_DEBUG_ERROR, "ERROR - LOCAL_PREF length was wrong -> %d\n", (unsigned int) length);
    } else {
	p += bgp_get_uint32(p, &value);
	session->attr.local_pref = value;
	bgp_debug(BGP_DEBUG_MSG, "0x%X", session->attr.local_pref);
    }
    
    return 0;
}


/*
**
*/
int bgp_decode_origin(struct bgp_session* session, uchar_t* p, uint32_t length) {
    uint8_t	value;
    
    if (length != sizeof(uint8_t)) {
	bgp_debug(BGP_DEBUG_ERROR, "ERROR - ORIGIN length was wrong -> %d\n", (unsigned int) length);
    } else {
	p += bgp_get_uint8(p, &value);
	if(value > bgp_max_ORIGIN) {
	    bgp_debug(BGP_DEBUG_ERROR, "error - out of bounds value from ORIGIN\n");
	} else {
	    bgp_debug(BGP_DEBUG_MSG, "%s (%d)", bgp_origin_names[value], value);
	    session->attr.origin = value;
	}
    }
    return 0;
}

/*
**
*/
int bgp_decode_communities(struct bgp_session* session, uchar_t* cm_data, uint32_t cm_length) {
    uchar_t*		p;
    uint32_t		community;
    int			n;
    
    session->attr.communities_len = 0;
    
    p = cm_data;
    n = cm_length / 4;
    while ((p - cm_data) < (signed int) cm_length) {
	p += bgp_get_uint32(p, &community);
	bgp_debug(BGP_DEBUG_MSG, "%d:%d ", (community & 0xFFFF0000) >> 16, (community & 0x0000FFFF));
	if(session->attr.communities_len < BGP_ATTR_MAX_COMMUNITIES) {
	    session->attr.communities[session->attr.communities_len++] = community;
	}
    }
    if(!(session->attr.communities_len < BGP_ATTR_MAX_COMMUNITIES)) {
	bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "communities length exceeded hardcoded maximum of %d - set truncated\n", BGP_ATTR_MAX_COMMUNITIES);
    }
    return 0;
}

/*
**
*/
int bgp_decode_nexthop(struct bgp_session* session, uchar_t* p, uint32_t nh_length) {
    uint32_t		next_hop;
    
    if (nh_length != 4) bgp_debug(BGP_DEBUG_ERROR, "ERROR - NEXT_HOP length was weird -> %d\n", (unsigned int) nh_length);
    p += bgp_get_uint32(p, &next_hop);
    bgp_debug(BGP_DEBUG_MSG, "%s", ip_addr2str(next_hop));
    session->attr.nexthop = next_hop;
    return 0;
}

/*
**
*/
int bgp_decode_aspath(struct bgp_session* session, uchar_t* asp_start, uint32_t asp_length) {
    uint8_t		paseg_type;
    uint8_t		paseg_length;
    uint16_t		as;
    uchar_t*		p;
    int			i;
    
    session->attr.aspath_len = 0;
    
    p = asp_start;
    /* more and more decoding */
    while ((p - asp_start) < (signed) asp_length) {
	p += bgp_get_uint8(p, &paseg_type);
	p += bgp_get_uint8(p, &paseg_length);
	
	/* XXX -- we probably should ignore AS SET. */
	if(paseg_type == bgp_AS_PATH_SET) {
	    bgp_debug(BGP_DEBUG_MSG | BGP_DEBUG_ASPATH, "{ ");
	}
	
	for(i = 0 ; i < paseg_length; i++) {
	    p += bgp_get_uint16(p, &as);
	    bgp_debug(BGP_DEBUG_MSG | BGP_DEBUG_ASPATH, "%d ", as);
	    /*
	    ** only store AS_SEQUENCE stuff and only to a limited length.
	    */
	    if(paseg_type == bgp_AS_PATH_SEQUENCE && session->attr.aspath_len < BGP_ATTR_MAX_ASPATH) {
		session->attr.aspath[session->attr.aspath_len++] = as;
	    }
	}
	if(paseg_type == bgp_AS_PATH_SET) {
	    bgp_debug(BGP_DEBUG_MSG | BGP_DEBUG_ASPATH, "}");
	}
    }
    if(!(session->attr.aspath_len < BGP_ATTR_MAX_ASPATH)) {
	bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "aspath length exceeded hardcoded maximum of %d - path truncated\n", BGP_ATTR_MAX_ASPATH);
    }
    return 0;
}

/*
** scary scary scary stuff..
** actually, not so scary now.. I've moved each attribute decode to separate functions..
**
*/
int bgp_decode_pa(struct bgp_session* session, uchar_t* pa_seq_start, uint32_t pa_seq_length) {
    
    unsigned char*		p;
    
    uint8_t			pa_flags;
    uint8_t			pa_type;
    uint8_t			pa_length;
    uchar_t*			pa_data;
    
    /*    fprintf(stderr, "bgp_decode_pa(0x%X, %d)\n", (unsigned int) pa_seq_start, pa_seq_length); */
    p = pa_seq_start;
    
    /* for each path attribute in the sequence */
    while((p - pa_seq_start) < (signed) pa_seq_length) {
	const char *name;
	/* one single pa */
	p += bgp_get_uint8(p, &pa_flags);
	p += bgp_get_uint8(p, &pa_type);
	p += bgp_get_uint8(p, &pa_length);
	
	if (pa_type > bgp_max_PA) {
	    name = "Unknown";
	} else {
	    name = bgp_pa_type_names[pa_type];
	}
	
	bgp_debug(BGP_DEBUG_MSG, "Rx UPDATE: PA: %11.11s (%02d) {", name, pa_type);
	
	if (pa_flags & 0x80) {
	    bgp_debug(BGP_DEBUG_MSG, "Opt,"); /* XXX are these bits the wrong way around? RFC is ambiguous*/
	} else {
	    bgp_debug(BGP_DEBUG_MSG, "Mand,");
	}
	
	if (pa_flags & 0x40) bgp_debug(BGP_DEBUG_MSG, "Trans,");
	if (pa_flags & 0x20) bgp_debug(BGP_DEBUG_MSG, "Ext,");
	if (pa_flags & 0x10) bgp_debug(BGP_DEBUG_MSG, "Part,");
	
	bgp_debug(BGP_DEBUG_MSG, "} -- ");
	pa_data = p;
	
	switch(pa_type) {
	    
	case bgp_PA_ORIGIN:
	    bgp_decode_origin(session, pa_data, pa_length);
	    break;
	    
	case bgp_PA_AS_PATH: 
	    bgp_decode_aspath(session, pa_data, pa_length);
	    break;
	    
	case bgp_PA_NEXT_HOP:
	    bgp_decode_nexthop(session, pa_data, pa_length);
	    break;
	    
	case bgp_PA_MULTI_EXIT_DISC:
	    bgp_decode_multi_exit_disc(session, pa_data, pa_length);
	    break;
	    
	case bgp_PA_LOCAL_PREF:
	    bgp_decode_local_pref(session, pa_data, pa_length);
	    break;
	    
	case bgp_PA_ATOMIC_AGGREGATE:
	    break;
	    
	case bgp_PA_AGGREGATOR:
	    bgp_decode_aggregator(session, pa_data, pa_length);
	    break;
	    
	case bgp_PA_COMMUNITIES:
	    bgp_decode_communities(session, pa_data, pa_length);
	    break;
	    
	default:
	    /* printf("Unknown Path Attributes Option %d\n", pa_type); */
	    
	}
	bgp_debug(BGP_DEBUG_MSG, "\n");
	
	p += pa_length;
    }
    
    return 0;
}

/*
**
**
*/
struct bgp_route* bgp_decode_nlri(struct bgp_session* session, uchar_t* block, uint32_t length) {
    
    struct bgp_route*		head;
    struct bgp_route*		route;
    struct bgp_route**		prevnext;
    uchar_t*			p;
    uint8_t			nbo_addr[4];
    int				i;
    
    route = 0;
    head = 0;
    prevnext = &head;
    p = block;
    bgp_debug(BGP_DEBUG_MSG, "Rx UPDATE: NLRI -- ");
    while((p - block) < (signed int) length) {
	*prevnext = (struct bgp_route*) malloc(sizeof(struct bgp_route));
	route = *prevnext;
	route->length = *p++;
	route->prefix = 0;
	memset(nbo_addr, 0, sizeof(uint32_t));	  /* bzero */
	for(i=0; i < BGP_PREFIX_STORAGE_SIZE(route->length); i++) {
	    nbo_addr[i] = *p++;
	}
	
	/* 
	** cast trick.
	*/
	route->prefix = ntohl(*(uint32_t*)nbo_addr);
	
	bgp_debug(BGP_DEBUG_MSG, "%s/%d ", ip_addr2str(route->prefix), route->length);
	session->remote.info_updates++;
	prevnext = &route->_next;
    }
    /* XXX --- guessing the end! */
    if(route) {
	route->_next = 0;
    }
    bgp_debug(BGP_DEBUG_MSG, "\n");
    return head;
}

/*
**
**
*/
struct bgp_route* bgp_decode_wd(struct bgp_session* session, uchar_t* block, uint32_t length) {
    
    struct bgp_route*		head;
    struct bgp_route*		route;
    struct bgp_route**		prevnext;
    
    uchar_t*			p;
    uint8_t			nbo_addr[4];
    int				i;
    
    route = 0;
    head = 0;
    prevnext = &head;
    p = block;
    
    bgp_debug(BGP_DEBUG_MSG, "Rx UPDATE: WD -- ");
    while((p - block) < (signed int) length) {
	*prevnext = (struct bgp_route*) malloc(sizeof(struct bgp_route));
	route = *prevnext;
	route->length = *p++;
	route->prefix = 0;
	
	memset(nbo_addr, 0, sizeof(uint32_t));	  /* bzero */
	
	for(i=0; i < BGP_PREFIX_STORAGE_SIZE(route->length); i++) {
	    nbo_addr[i] = *p++;
	}
	
	/* 
	** lame trick.
	*/
	route->prefix = ntohl(*(uint32_t*)nbo_addr);
	
	bgp_debug(BGP_DEBUG_MSG, "%s/%d ", ip_addr2str(route->prefix), route->length);
	session->remote.info_withdrawns++;
	prevnext = &route->_next;
    }
    if(route) {
	route->_next = 0;
    }
    bgp_debug(BGP_DEBUG_MSG, "\n");
    return head;
}

/*
**
**
*/
void bgp_decode_message(struct bgp_session* session) {
    uint32_t		marker[4];
    struct bgp_msg*	msg;
    
    memset(&session->attr, 0, sizeof(struct bgp_attr));
    
    msg = &session->msg;
    
    /* bgp_print_message(msg->raw, msg->size);  */
    /*
    ** Decode the Header.
    */
    msg->p += bgp_get_uint32(msg->p, &marker[0]);			/* marker */
    msg->p += bgp_get_uint32(msg->p, &marker[1]);			/* marker */
    msg->p += bgp_get_uint32(msg->p, &marker[2]);			/* marker */
    msg->p += bgp_get_uint32(msg->p, &marker[3]);			/* marker */
    
    msg->p += bgp_get_uint16(msg->p, &msg->size);			/* message length including header */
    msg->p += bgp_get_uint8(msg->p, &msg->type);			/* message type */
    
    if (~marker[0] & ~marker[1] & ~marker[2] & ~marker[3]) {
	bgp_debug(BGP_DEBUG_ERROR, "broken marker, lost sync\n");
	bgp_print_message(msg->raw, BGP_HEADER_LENGTH);
	session->event = bgp_EVENT_STOP;
	return;
    }
    /*
    ** Decode the rest of the message.
    */
    /* "received %s (%d) message from session %s\n", bgp_msg_type_names[msg->type], msg->type, session->remote.hostname); */
    
    if(msg->type <= bgp_max_MSG) {
	bgp_debug(BGP_DEBUG_MSG | BGP_DEBUG_TIMESTAMP, "Received %s message\n", bgp_msg_type_names[msg->type]);
    }
    switch(msg->type) {
	
    case bgp_MSG_OPEN:
	msg->p += bgp_get_uint8(msg->p, &session->remote.version);
	msg->p += bgp_get_uint16(msg->p, &session->remote.as);
	msg->p += bgp_get_uint16(msg->p, &session->remote.hold_time);
	msg->p += bgp_get_uint32(msg->p, &session->remote.id);
	msg->p += bgp_get_uint8(msg->p, &session->remote.options_size);
	
	bgp_debug(BGP_DEBUG_MSG, "Rx OPEN: connection from %s (%s)\n", session->remote.hostname, ip_addr2str(session->remote.addr));
	bgp_debug(BGP_DEBUG_MSG, "Rx OPEN: remote AS %d\n", session->remote.as);
	bgp_debug(BGP_DEBUG_MSG, "Rx OPEN: remote holdtime %d\n", session->remote.hold_time);
	bgp_debug(BGP_DEBUG_MSG, "Rx OPEN: remote id = %s\n", ip_addr2str(session->remote.id));
	bgp_debug(BGP_DEBUG_MSG, "Rx OPEN: options length = %d\n", session->remote.options_size);
	
	break;
	
    case bgp_MSG_NOTIFICATION:
	/** woa! **/
	{
	    msg->p += bgp_get_uint8(msg->p, &session->remote.error);
	    msg->p += bgp_get_uint8(msg->p, &session->remote.sub_error);
	    bgp_debug(BGP_DEBUG_MSG, "Rx NOTIFICATION: type %d, subtype %d\n", session->local.error, session->local.sub_error);
	    /*
	    ** XXX - does the state machine handle these correctly?
	    */
	}
	break;
	
    case bgp_MSG_UPDATE: {
	
	struct bgp_route*	wd;
	struct bgp_route*	nlri;
	
	uint16_t	block_wd_len;
	uint16_t	block_pa_len;
	uint16_t	block_nlri_len;
	uchar_t*	block_wd;
	uchar_t*	block_pa;
	uchar_t*	block_nlri;
	
	msg->p += bgp_get_uint16(msg->p, &block_wd_len);
	block_wd = msg->p;
	msg->p += block_wd_len;
	wd = bgp_decode_wd(session, block_wd, block_wd_len);
	
	msg->p += bgp_get_uint16(msg->p, &block_pa_len);
	block_pa = msg->p;
	msg->p += block_pa_len;
	bgp_decode_pa(session, block_pa, block_pa_len);
	
	block_nlri_len = msg->size - BGP_HEADER_LENGTH - 4 - block_pa_len - block_wd_len;
	block_nlri = msg->p;
	nlri = bgp_decode_nlri(session, block_nlri, block_nlri_len);
	
	/*
	** Enter the routing informatin into the database.
	*/
	maintain_routes(&session->attr, nlri, wd);
	
	/*
	** free the memory used for routing information.
	*/ 
	while(wd) {
	    struct bgp_route* here;
	    here = wd;
	    wd = wd->_next;
	    free(here);
	}

	while(nlri) {
	    struct bgp_route* here;
	    here = nlri;
	    nlri = nlri->_next;
	    free(here);
	}
	
	break;
    }
    case bgp_MSG_KEEPALIVE:
	break;
	
    }
    return;
}



/*
** Format a raw message.
**
*/
static void bgp_encode_message(struct bgp_session* session, enum bgp_msg_type_e type) {
    struct bgp_msg*		msg;
    struct bgp_tlv*		parm;
    
    msg = &session->msg;
    msg->type = type;
    msg->p = msg->raw;
    
    parm = session->remote.options;
    /*    printf("bgp_encode_message\n"); */
    /* build the header */
    msg->p += bgp_set_uint32(msg->p, 0xFFFFFFFF);			/* marker */
    msg->p += bgp_set_uint32(msg->p, 0xFFFFFFFF);			/* marker */
    msg->p += bgp_set_uint32(msg->p, 0xFFFFFFFF);			/* marker */
    msg->p += bgp_set_uint32(msg->p, 0xFFFFFFFF);			/* marker */
    msg->p += bgp_set_uint16(msg->p, (uint16_t) 0x0000);		/* we'll fix up the length value later */
    msg->p += bgp_set_uint8(msg->p, msg->type);			/* message type */
    
    if(msg->type <= bgp_max_MSG) {
	bgp_debug(BGP_DEBUG_MSG | BGP_DEBUG_TIMESTAMP, "Sending %s message\n", bgp_msg_type_names[msg->type]);
    }
    
    switch (msg->type) {
	
    case bgp_MSG_OPEN:
	/* build the open message */
	msg->p += bgp_set_uint8(msg->p, session->local.version);
	msg->p += bgp_set_uint16(msg->p, session->local.as);
	msg->p += bgp_set_uint16(msg->p, session->local.hold_time);
	msg->p += bgp_set_uint32(msg->p, session->local.id); /* flip it back again.. */
	msg->p += bgp_set_uint8(msg->p, session->local.options_size);
	
	bgp_debug(BGP_DEBUG_MSG, "Tx OPEN: connecting to %s (%s)\n", session->local.hostname, ip_addr2str(session->local.addr));
	bgp_debug(BGP_DEBUG_MSG, "Tx OPEN: local AS %d\n", session->local.as);
	bgp_debug(BGP_DEBUG_MSG, "Tx OPEN: local holdtime %d\n", session->local.hold_time);
	bgp_debug(BGP_DEBUG_MSG, "Tx OPEN: local id = %s\n", ip_addr2str(session->local.id));
	bgp_debug(BGP_DEBUG_MSG, "Tx OPEN: options length = %d\n", session->local.options_size);
	
	while (parm) {
	    /* churn through options */
	    bgp_debug(BGP_DEBUG_ERROR, "WARNING: We don't support optional parameters\n");
	}
	
	/** add parameters here **/
	break;
	
    case bgp_MSG_UPDATE:
	bgp_debug(BGP_DEBUG_MSG, "\tbgp_MSG_UPDATE message being formatted\n");
	/* we don't support the sending of any of UPDATEs */
	break;
	
    case bgp_MSG_NOTIFICATION:
	bgp_debug(BGP_DEBUG_MSG, "Tx NOTIFICATION:\n");
	bgp_debug(BGP_DEBUG_MSG, "\tbgp_MSG_NOTIFICATION message being formatted\n");
	msg->p += bgp_set_uint8(msg->p, session->local.error);
	msg->p += bgp_set_uint8(msg->p, session->local.sub_error);
	
	break;
	
    case bgp_MSG_KEEPALIVE:
	break;
    }
    msg->size = msg->p - msg->raw; 
    bgp_set_uint16(&msg->raw[16], msg->size); /* go back and write the message size in. */
}



/*
** Print the raw message in hex.  If you're very sick, you can read these.
** 
*/
static void bgp_print_message(uchar_t* p, uint32_t length) {
    unsigned int	offset;
    
    offset = 0;
    while(offset < length) {
	if(!(offset%4)) bgp_debug(BGP_DEBUG_MSG, "\n\t%02d: ", offset);
	bgp_debug(BGP_DEBUG_MSG, "0x%02X ", (unsigned char) p[offset]);
	offset++;
    }
    bgp_debug(BGP_DEBUG_MSG, "\n");
    return;
}    


/*
** Encoding functions that handle Host to Network Byte Order.
**
*/
static __inline__ size_t bgp_set_uint8(uchar_t* p, uint8_t field) {
    
    p[0] = (uint8_t) field & 0xFF;
    
    return sizeof(uint8_t);
}

static __inline__ size_t bgp_set_uint16(uchar_t* p, uint16_t field) {
    
    p[0] = (uint8_t) (field >> 8) & 0xFF;
    p[1] = (uint8_t) (field & 0xFF);
    
    return sizeof(uint16_t);
}

static __inline__ size_t bgp_set_uint32(uchar_t* p, uint32_t field) {
    
    p[0] = (uint8_t) (field >> 24) & 0xFF;
    p[1] = (uint8_t) (field >> 16) & 0xFF;
    p[2] = (uint8_t) (field >> 8) & 0xFF;
    p[3] = (uint8_t) (field & 0xFF);
    
    return sizeof(uint32_t);
}

/*
** Decoding functions that handle Network to Host Byte Order.
*/
static __inline__ size_t bgp_get_uint8(uchar_t* p, uint8_t* v) {
    *v = p[0];
    return sizeof(uint8_t);
}

static __inline__ size_t bgp_get_uint16(uchar_t* p, uint16_t* v) {
    *v =  p[0] << 8;
    *v |= p[1];
    return sizeof(uint16_t);
}

static __inline__ size_t bgp_get_uint32(uchar_t* p, uint32_t* v) {
    *v =  p[0] << 24;
    *v |= p[1] << 16;
    *v |= p[2] << 8;
    *v |= p[3];
    return sizeof(uint32_t);
}

static void bgp_print_state(struct bgp_session *session) {
    struct timeval	tv;
    
    printf("STATE: %s(%d)\n", bgp_state_names[session->state], session->state);
    printf("EVENT: %s(%d)\n", bgp_event_names[session->event], session->event);
    printf("TIME: %d\n", (unsigned int) tv.tv_sec);
    printf("ACTIVE TIMERS: ");
    if(session->local.retry_expire) printf("retry_expire=%d ", (int) session->local.retry_expire);
    if(session->local.keepalive_expire) printf("keepalive_expire=%d ", (int) session->local.keepalive_expire);
    if(session->local.hold_expire) printf("hold_expire=%d ", (int) session->local.hold_expire);
    printf("\n");
    printf("LOCAL:\n");
    printf(" HOSTNAME: %s (%s)\n", session->local.hostname, ip_addr2str(session->local.addr));
    printf(" VERSION: %d\n", session->local.version);
    printf(" PORT: %d\n", session->local.port);
    printf(" AS: AS%d\n", session->local.as);
    printf(" KEEPALIVE_TIME: %d\n", (unsigned int) session->local.keepalive_time);
    printf(" RETRY_TIME: %d\n", (unsigned int) session->local.retry_time);
    printf(" HOLD_TIME: %d\n", (unsigned int) session->local.hold_time);
    printf(" KEEPALIVE_EXPIRE: %d\n", (unsigned int) session->local.keepalive_expire);
    printf(" RETRY_EXPIRE: %d\n", (unsigned int) session->local.retry_expire);
    printf(" HOLD_EXPIRE: %d\n", (unsigned int) session->local.hold_expire);
    printf(" ERROR: %d\n", session->local.error);
    printf(" SUB_ERROR: %d\n", session->local.sub_error);
    printf("REMOTE:\n");
    printf(" HOSTNAME: %s (%s)\n", session->remote.hostname, ip_addr2str(session->remote.addr));
    printf(" VERSION: %d\n", session->remote.version);
    printf(" PORT: %d\n", session->remote.port);
    printf(" AS: AS%d\n", session->remote.as);
    printf(" KEEPALIVE_TIME: %d\n", (unsigned int) session->remote.keepalive_time);
    printf(" RETRY_TIME: %d\n", (unsigned int) session->remote.retry_time);
    printf(" HOLD_TIME: %d\n", (unsigned int) session->remote.hold_time);
    printf(" KEEPALIVE_EXPIRE: %d\n", (unsigned int) session->remote.keepalive_expire);
    printf(" RETRY_EXPIRE: %d\n", (unsigned int) session->remote.retry_expire);
    printf(" HOLD_EXPIRE: %d\n", (unsigned int) session->remote.hold_expire);
    printf(" ERROR: %d\n", session->remote.error);
    printf(" SUB_ERROR: %d\n", session->remote.sub_error);
    
    return;
}

/*
** build a debug level from a string
** eg, all,no-timers or route,timers,fsm, etc.
*/

static uint32_t	bgp_parse_debug_string(const char* debug_str, uint32_t level) {
    char*	str;
    char*	field;
    
    if(debug_str == NULL) {
	return 0;
    }
    
    str = strdup(debug_str);
    if(str == NULL) {
	return 0;
    }
    
    /*
    ** separate by commas.
    */
    field = strtok(str, ",");
    while(field) {
	const char*	name;
	int		i;
	int		negate;
	
	/*
	** lookup the field in the list.
	*/
	i = 0;
	name = debug_names[i];
	negate = 0;
	if (strncmp(field, "no-", 3) == 0) {
	    negate = 1;
	    field = &field[3];
	}
	
	if (strcmp(field, "all") == 0) {
	    if(!negate)
		level = ~0;
	} else {
	    while(name) {
		if (strcmp(name, field) == 0) {
		    if(negate == 0) {
			level |= (1u << i);
		    } else {
			level &= ~(1u << i);
		    }
		    break;
		}
		i++;
		name = debug_names[i];
	    }
	}
	if(name == NULL) {
	    free(str);
	    return 0;
	}
	field = strtok(NULL, ",");
    }
    free(str);
    return level;
}


/*
**
*/
static void	bgp_print_debug_settings(uint32_t level) {
    int			i;
    const char*		name;
    
    printf("activated debug levels are - ");
    i = 0;
    while(debug_names[i] != NULL) {
	name = debug_names[i];
	if(level & (1<<i)) {
	    printf("%s,", name);
	}
	i++;
    }
    printf("\ndisabled debug levels are - ");
    i = 0;
    while(debug_names[i] != NULL) {
	name = debug_names[i];
	if(!(level & (1<<i))) {
	    printf("%s,", name);
	}
	i++;
    }
    printf("\n");
    return;
}	
    

static void	bgp_print_usage(void) {
    int			i;
    const char*		name;
    
    fprintf(stderr, "usage: bgp [-c<path>] [-d<options>] [-p <port>] [-t <holdtimer>] [-v] [-l <ipaddess>] -a <asnumber> <hostname>\n");
    fprintf(stderr, "\n\nExplanation of Options\n");
    fprintf(stderr, "-------------------------\n\n");
    fprintf(stderr, "-d<debug_options> \n");
    fprintf(stderr, "                  where debug_options is any comma separated list of the following -\n");
    fprintf(stderr, "                  ");
    i = 0;
    name = debug_names[i];
    while(name) {
	fprintf(stderr, "%s,", name);
	i++;
	name = debug_names[i];
    }    
    fprintf(stderr, "all\n");
    fprintf(stderr, "                  or with any option prefixed with 'no-' to disable that option\n\n");
    fprintf(stderr, "-l <ipaddress>    The local hostname or IP address to be used for the BGP connection\n\n");
    fprintf(stderr, "                  By default the IP address matching the hostname will be used\n\n");
    fprintf(stderr, "-a <asnumber>     A mandatory argument defining the Automous Systems number of this host\n\n");
    fprintf(stderr, "-p <port>         The remote TCP port for the BGP connection.  Port 179 by default\n\n");
    fprintf(stderr, "-c <path>         The pathname of the base directory to run this in\n");
    fprintf(stderr, "<hostname>        The remote hostname of IP address of the BGP connection\n\n");
}


static void	bgp_signal_init(void) {
    
    struct sigaction	 sa;
    struct sigaction	 osa;
    
    last_signal = 0;
    
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = bgp_signal_handler;
    sa.sa_flags =  0; /* SA_RESTART; */
    sigaction(SIGHUP, &sa, &osa);
    sigaction(SIGINT, &sa, &osa);
    sigaction(SIGTERM, &sa, &osa);
    sigaction(SIGQUIT, &sa, &osa);
    sigaction(SIGUSR1, &sa, &osa);
    sigaction(SIGUSR2, &sa, &osa);
}

/*
** not allowed to use printf in here.
*/
static void	bgp_signal_handler(int sig, siginfo_t* sip, void* uap) {
    
    pending.signal = sig;
    pending.time = time((time_t*) NULL);
    
    if(sig == SIGHUP) {
	pending.event = bgp_EVENT_STOP;
    } else {
	pending.event = bgp_EVENT_TERMINATE;
    }
    
    return;
}

static void	bgp_exit(void) {
    exit(1);
}
