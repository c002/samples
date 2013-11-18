/* 	$Id: ip2cidr.h,v 1.1.1.1 2004/05/29 09:06:53 harry Exp $	 */

#ifndef IP2CIDR_H
#define IP2CIDR_H

#include "btalib.h"

#define	ADDRBITS	32
#define CIDRMEMPATH	"/var/tmp/cidrmemdb"

#define	IP_FILLER	0x00000001
#define IP_EXPIRED	0x00000002

/*
 * If the watchdog timer is not update for more than this many
 * seconds by a provider, a consumer should complain loudly, cause
 * it probably means the provider has died/broken/hung
 */
#define CIDR_WATCHDOG_LATE_TIME 100

/*
** A structure to pass pointer and length
** of userdata between the external application
** and ip2cidr functions.
*/

typedef struct cidr_userdata {
    uint32_t		prefix;			/* redundent network information */
    uint32_t		length;
    void*		datap;			/* pointer to user supplied data */
    size_t		size;			/* length of the data */
} cidr_userdata_t;

/*
** scary internal scructures.
*/
typedef struct cidrnode {
    uint32_t		prefix;			/* This node's prefix */
    uint32_t		suffix;			/* This node's suffix */
    cidr_userdata_t	userdata;		/* User supplied data */
    uint32_t		flags;			/* Flags - filler node, expired */
    
    /* If there are sub-nodes, then prefix and suffix are used */
    /* to compare against which path to take */
    
    struct cidrnode*	left;
    struct cidrnode*	right;
} cidrnode;

typedef struct suffixnode {
    uint32_t		suffix;
    uint32_t		size;
    struct cidrnode**	base;
    struct suffixnode*	next;
} suffixnode;

typedef struct cidrconf {
    suffixnode*		suffixes;
    int			totalfiller;
    
    /* Set to current time by providers, checked by consumers */
    time_t		watchdog_time;
} cidrconf;

extern int		cidr_init(const char*, int existing);
extern int		cidr_expireall(void);
extern int		cidr_purge(void (*)(cidr_userdata_t*));
extern int		cidr_update(cidr_userdata_t*, void (*)(cidr_userdata_t*));
extern int		cidr_insert(cidr_userdata_t*, void (*)(cidr_userdata_t*));
extern int		cidr_remove(uint32_t, uint32_t, void (*)(cidr_userdata_t*));
extern int		cidr_lookup(cidr_userdata_t*);
extern void		cidr_watchdog_tick(void);
extern int		cidr_watchdog_ok(void);
extern void		cidr_printtree(void);
extern void		cidr_printtree_sub(cidrnode*, int);
extern void		cidr_checktree(void);
extern void		cidr_checktree_sub(cidrnode*, cidrnode *);
extern int		cidr_lookup_tree(uint32_t, uint32_t, void (*)(cidr_userdata_t*));
extern int		cidr_lookup_tree_sub(cidrnode*, uint32_t, uint32_t, uint32_t, void (*)(cidr_userdata_t*));
extern void		cidr_checktree_split(cidrnode* base, cidrnode* cnp, uint32_t prefix, uint32_t length);

#endif
