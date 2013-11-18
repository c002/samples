/* -*-mode: c; fill-column: 75; comment-column: 50; -*- */

/*
** (c) 1998 Connect.com.au.
**
** Jim Crumpler
**
** support and interfaces file for bgp.
**
**
**
**
*/

#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>
#include <syslog.h>

#include "bgp_api.h"
#include "btalib.h"
#include "ip2cidr.h"
#include "mpmalloc.h"

#include "exception.h"

uint32_t bgp_debug_level;


const char*	debug_names[] = {"log",		  /* 0 */
				 "route",	  /* 1 */
				 "error",	  /* 2 */
				 "general",	  /* 3 */
				 "fsm",		  /* 4 */
				 "msg",		  /* 5 */
				 "packet",	  /* 6 */
				 "verbose",	  /* 7 */
				 "stdout",	  /* 8 */
				 "timers",	  /* 9 */
				 "default",	  /* 10 */
				 "info",	  /* 11 */
				 "timestamp",	  /* 12 */
				 "warning",	  /* 13 */
				 "cidrdb",	  /* 14 */
				 "aspath",	  /* 15 */
				 "expire",	  /* 16 */
				 NULL};		  /* 17 */

#define DB_STATUS_OK	0
#define DB_STATUS_FUBAR	-1

static FILE*		debugf = 0;
static unsigned int	db_status;

/*
** initialise the database connection.
** arg is just a descriptive name to pass
** to any debug routines.. argv[0] is suitable.
**
*/ 
int bgp_db_init(const char* arg) {
    int retval;
    mpm_debug_init(arg);
    retval = cidr_init("bgp",0);
    if (retval <0 )
	db_status = DB_STATUS_FUBAR;
    else
	return retval;

    return db_status;
}
/* Same as bgp_db_init, but will only attach to an existing segment 
   and note try create a new on */
int bgp_db_attach(const char* arg) {
    int retval;
    mpm_debug_init(arg);
    retval = cidr_init("bgp",1);
    if (retval <0 )
	db_status = DB_STATUS_FUBAR;
    else
	return retval;

    return db_status;
}

int bgp_db_close(void) {
    
    return 1;
}

/*
** 
*/
int bgp_add_route(uint32_t prefix, uint32_t length, struct bgp_attr* attr) {
    cidr_userdata_t	userdata;
    unsigned int	i;
    int			retval;
    
    if(db_status != DB_STATUS_OK) {
	return 0;
    }
    
    bgp_debug(BGP_DEBUG_ROUTE, "adding %s/%d, (ref=%d->%d) (attr=0x%X), ",
	      ip_addr2str(prefix), length,
	      attr->_refcount, attr->_refcount+1,
	      attr);
    bgp_debug(BGP_DEBUG_ROUTE, "nexthop=%s, AS_PATH=", ip_addr2str(attr->nexthop));
    for(i = 0 ; i < attr->aspath_len ; i++) {
	bgp_debug(BGP_DEBUG_ROUTE, "%d", (unsigned short) attr->aspath[i]);
	if(i < attr->aspath_len - 1) {
	    bgp_debug(BGP_DEBUG_ROUTE, "_");
	}
    }
    if(attr->communities_len) {
	bgp_debug(BGP_DEBUG_ROUTE, ", communities=");
	for(i = 0 ; i < attr->communities_len ; i++) {
	    bgp_debug(BGP_DEBUG_ROUTE, " 0x%08x ", attr->communities[i]);
	    if(i < attr->communities_len - 1) {
		bgp_debug(BGP_DEBUG_ROUTE, "_");
	    }
	}
    }
    bgp_debug(BGP_DEBUG_ROUTE, "\n");
    
    /*
    ** cidr_insert will copy the structure
    */
    userdata.prefix = prefix;
    userdata.length = length;
    userdata.datap = attr;
    userdata.size = sizeof(struct bgp_attr);
    /* retval = cidr_insert(prefix, length, &userdata); */
    retval = cidr_update(&userdata, bgp_decrement_reference);
    if(retval < 0) {
	bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_WARNING | BGP_DEBUG_TIMESTAMP, "failed to add route %s/%d - %s\n", ip_addr2str(prefix), length, strerror(errno));
	return -1;
    }
    
    attr->_refcount++;
    return 0;
}

/*
** bgp_lookup_route will query a route by the given prefix/length pair in the argument.
**
** The resulting route will be stored in the argument's prefix/length pair,
** and the resulting attribute will be copied in the address pointed to by
** the attr member.
**
*/
int bgp_lookup_route(struct bgp_route_info*	route) {    
    cidr_userdata_t		userdata;
    int				retval;
    
    if(db_status != DB_STATUS_OK) {
	return 0;
    }
    
    if(route == NULL || route->attr == NULL) {
	bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "cidr_lookup() was passed a NULL attr address\n");
	return 0;
    }
    
    /*
    ** copy the lookup information to the ip2cidr userdata structure.
    */
    userdata.prefix = route->prefix;
    userdata.length = route->length;
    userdata.datap  = route->attr;
    userdata.size = sizeof(struct bgp_attr);
    
    /* memset(route, 0, sizeof(struct bgp_route_info)); */
    bgp_debug(BGP_DEBUG_VERBOSE, "looking up route %s/%d\n", ip_addr2str(userdata.prefix), userdata.length);
    retval = cidr_lookup(&userdata);
    if(retval < 0) {
	return 0;
    } else {
	if(userdata.datap == NULL) {
	    bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "cidr_lookup returned userdata, but it contains a datap of NULL\n");
	    return 0;
	}
	bgp_debug(BGP_DEBUG_VERBOSE, "returned with route %s/%d\n", ip_addr2str(userdata.prefix), userdata.length);
	/*
	** attr will be filled, and the actual route information is copied.
	*/
	route->prefix = userdata.prefix;
	route->length = userdata.length;
	route->attr = userdata.datap;
	return 1;
    }
}
/*
** delete one route from the database.
*/
int bgp_delete_route(uint32_t prefix, uint32_t length) {
    int			retval;
    
    if(db_status != DB_STATUS_OK) {
	return 0;
    }
    
    retval = cidr_remove(prefix, length, bgp_decrement_reference);
    if(retval < 0) {
	bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_WARNING | BGP_DEBUG_TIMESTAMP, 
		  "WARNING - withdrawn route not in the database (%s/%d) - %s\n", ip_addr2str(prefix), length, strerror(errno));
    }
    return 0;
}

/*
** This is used by bgp_decode_message..
**
** It passes a static buffer of attributes, a linked list of reachable routes, 
** and a linked list of unreachable routes.
**
** maintain_routes() adds these route changes to the ip2cidr database.
**
** The attributes are maintained as one object, with each reference 
** from a route being accounted for by a reference count, which is incrememted for 
** added routes and decremented for deleted routes.
**
** The attribute is deleted when the reference count is zero.
*/
int maintain_routes(struct bgp_attr* attrbuf, struct bgp_route* nlri, struct bgp_route* wd) {
    struct bgp_attr*	attr;
    
    /*
    ** Withdrawn Routes.
    */
    while(wd) {
	bgp_delete_route(wd->prefix, wd->length);
	wd = wd->_next;
    }
    
    /*
    ** Add networks one at a time.
    */
    if(nlri) {
	/*
	** make a shared memory copy of the attributes.
	*/
	attr = (struct bgp_attr*) mpm_alloc(sizeof(struct bgp_attr));
	memcpy(attr, attrbuf, sizeof(struct bgp_attr));
	
	/* attr->aspath = (uint32_t*) mpm_alloc(sizeof(uint32_t) * attr->aspath_len); */
	/* attr->communities = (uint32_t*) mpm_alloc(sizeof(uint32_t) * attr->communities_len); */
	
	/* memcpy(attr->aspath, attrbuf->aspath, sizeof(uint32_t) * attr->aspath_len); */
	/* memcpy(attr->communities, attrbuf->communities, sizeof(uint32_t) * attr->communities_len); */
	
	while(nlri) {
	    bgp_add_route(nlri->prefix, nlri->length, attr);
	    nlri = nlri->_next;
	}
    }
    return 0;
}

/*
** open a debug file for this client.
*/
void bgp_debug_init(const char* filename) {
    
    openlog("bgp",LOG_PID,LOG_LOCAL3);
    debugf = fopen(filename, "a+");
    
    return;
    
}
/*
** output debug information, if it has a priority higher than
** the current debug level.
*/
void bgp_debug(uint32_t level, const char *format, ...)
{
    
    char*	timestr;
    time_t	t;
    
    if(debugf == 0) {
	return;
    }
    
    /*
    ** filter unwanted levels.
    */
    if(bgp_debug_level & level) {
	va_list ap;
	/* va_start(ap,); */
	va_start(ap,*format);
	
	if(bgp_debug_level & BGP_DEBUG_STDOUT) {
	    level |= BGP_DEBUG_STDOUT;
	}
	
	/*
	** duplicate to stdout for errors.
	*/
	if(level & BGP_DEBUG_ERROR) {
	    level |= BGP_DEBUG_STDOUT;
	}
	
	/*
	** prefix with timestamp, if required.
	*/
	if(level & BGP_DEBUG_TIMESTAMP) {
	    t = time((time_t*) NULL);
	    timestr = ctime(&t);
	    if(timestr == NULL) 
		return ;
	    timestr[24] = '\0';
	    fprintf(debugf, "%s - ", timestr);
	    if(level & BGP_DEBUG_STDOUT) {
		fprintf(stdout, "%s - ", timestr);
	    }
	}
	
	vfprintf(debugf, format, ap);
	fflush(debugf);
	syslog(LOG_DEBUG,debugf);
	/* level |= BGP_DEBUG_STDOUT; */
	if(level & BGP_DEBUG_STDOUT) {
	    vfprintf(stdout, format, ap);
	    fflush(stdout);
	}

	va_end(ap);


    }
}

/*
** decrement a reference counter to an attribute
** If the ref count is zero, the memory is freed (shared mem)
** 
** This function is used as a callback for the ip2cidr functions.
**
** This function is 90% debug output to show the network being deleted :-)
*/
void bgp_decrement_reference(cidr_userdata_t* udp) {
    struct bgp_attr*	attr;
    uint32_t		prefix;
    uint32_t		length;
    unsigned int	i;
    
    prefix = udp->prefix;
    length = udp->length;
    attr = (struct bgp_attr*) udp->datap;
    
    if(udp == NULL) {
	bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "bgp_decrement_reference - was passed NULL data pointer\n");
	return;
    }
    if(udp->datap == NULL) {
	bgp_debug(BGP_DEBUG_ERROR | BGP_DEBUG_STDOUT | BGP_DEBUG_TIMESTAMP, "bgp_decrement_reference - was passed NULL userdata pointer\n");
	return;
    }
    
    if(udp->size != sizeof(struct bgp_attr)) {
/* harryr */
        bgp_debug(BGP_DEBUG_STDOUT | BGP_DEBUG_INFO | BGP_DEBUG_TIMESTAMP, 
                          "Aborting. udp->size != sizeof(struct bgp_attr)\n");

	abort();
    }
    /*
    ** print out the network being deleted.
    */
    {
	bgp_debug(BGP_DEBUG_ROUTE, "del %s/%d, (ref=%d->%d), ", ip_addr2str(prefix), length, attr->_refcount, attr->_refcount - 1);
	bgp_debug(BGP_DEBUG_ROUTE, "about to del %s/%d (attr=0x%X)\n", ip_addr2str(prefix), length, attr);
	
	bgp_debug(BGP_DEBUG_ROUTE, "nexthop=%s, AS_PATH=", ip_addr2str(attr->nexthop));
	for(i = 0 ; i < attr->aspath_len ; i++) {
	    bgp_debug(BGP_DEBUG_ROUTE, "%d", (unsigned short) attr->aspath[i]);
	    if(i < attr->aspath_len - 1) {
		bgp_debug(BGP_DEBUG_ROUTE, "_");
	    }
	}
	if(attr->communities_len) {
	    bgp_debug(BGP_DEBUG_ROUTE, "communities=");
	    for(i = 0 ; i < attr->communities_len ; i++) {
		bgp_debug(BGP_DEBUG_ROUTE, "0x%08x ", attr->communities[i]);
	    }
	}
	bgp_debug(BGP_DEBUG_ROUTE,"\n");
    }
    
    attr->_refcount--;
    
    if(!attr->_refcount) {
	/* if(attr->communities) {
	     mpm_free(attr->communities);
	}
	if(attr->aspath) {
	    mpm_free(attr->aspath);
	}
	*/
	mpm_free(attr);
    }
    return;
}

