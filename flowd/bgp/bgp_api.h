#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifndef __BGP_API_H__
#define __BGP_API_H__

#include "btalib.h"
#include "ip2cidr.h"


/*
** These defintions _must_ match the strings in bgp_api.c
*/
#define BGP_DEBUG_LOG		(1<<0)
#define BGP_DEBUG_ROUTE		(1<<1)
#define BGP_DEBUG_ERROR		(1<<2)
#define BGP_DEBUG_GENERAL	(1<<3)
#define BGP_DEBUG_FSM		(1<<4)
#define BGP_DEBUG_MSG		(1<<5)
#define BGP_DEBUG_PACKET	(1<<6)
#define BGP_DEBUG_VERBOSE	(1<<7)
#define BGP_DEBUG_STDOUT	(1<<8)
#define BGP_DEBUG_TIMERS	(1<<9)
#define BGP_DEBUG_DEFAULT	(1<<10)
#define BGP_DEBUG_INFO		(1<<11)
#define BGP_DEBUG_TIMESTAMP	(1<<12)
#define BGP_DEBUG_WARNING	(1<<13)
#define BGP_DEBUG_CIDRDB	(1<<14)
#define BGP_DEBUG_ASPATH	(1<<15)
#define BGP_DEBUG_EXPIRE	(1<<16)


enum bgp_origin_e {
    bgp_min_ORIGIN		= 0,
    bgp_ORIGIN_IGP		= 0,
    bgp_ORIGIN_EGP		= 1,
    bgp_ORIGIN_INCOMPLETE	= 2,
    bgp_max_ORIGIN		= 2
};



extern int gethostname(char *, int);

extern uint32_t		bgp_debug_level;
extern const char*	debug_names[];

#define BGP_ATTR_MAX_ASPATH		26
#define BGP_ATTR_MAX_COMMUNITIES	16

struct bgp_attr {
    uint32_t			communities[BGP_ATTR_MAX_COMMUNITIES];
    uint32_t			communities_len;
    uint16_t			aspath[BGP_ATTR_MAX_ASPATH];
    uint32_t			aspath_len;
    uint32_t			nexthop;
    uint32_t			origin;
    uint32_t			multiexit;
    uint32_t			local_pref;
    uint16_t			aggregator_as;
    uint32_t			aggregator_ip;
    uint32_t			_refcount;
};

struct bgp_route {
    uint32_t			prefix;
    uint8_t			length;
    struct bgp_route*		_next;			/* internal chaining only */
};

struct bgp_route_info {
    uint32_t			prefix;
    uint32_t			length;
    struct bgp_attr*		attr;
};

extern int			bgp_add_route(uint32_t, uint32_t, struct bgp_attr*);
extern int			bgp_lookup_route(struct bgp_route_info*);
extern int			bgp_delete_route(uint32_t, uint32_t);
extern int			maintain_routes(struct bgp_attr*, struct bgp_route*, struct bgp_route*);
extern void			bgp_debug(uint32_t, const char*, ...);
extern int                      bgp_db_attach(const char*);
extern int			bgp_db_init(const char*);
extern void			bgp_debug_init(const char*);
extern void			bgp_decrement_reference(cidr_userdata_t* udp);
extern int			bgp_db_close(void);
#endif /*__BGP_API_H__ */


