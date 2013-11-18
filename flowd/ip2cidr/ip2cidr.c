/*	Author: Stew Forster
 *
 *	Core module for conversion of an IP address to a CIDR address
 *
 * 	$Id: ip2cidr.c,v 1.1.1.1 2004/05/29 09:06:53 harry Exp $	 
 *
 */

/*
** comments, modifications and fixes. - 1998-11-12
** Jim.Crumpler@unicity.com.au.
** 
** 
** 
** 
*/

#if defined(DEBUG_IP2CIDR) 
#define DFPRINTF(args)	fprintf args
#else
#define DFPRINTF(args) 
#endif

#include	<sys/types.h>
#include	<errno.h>
#include	<string.h>
#include	<stdio.h>
#include	<assert.h>
#include	"ip2cidr.h"
#include	"mpmalloc.h"
#include	"btalib.h"

/*
** a set bunch of trees hang off a bunch of arrays.  The array sizes
** are 2^ or the following numbers, which represents the number of 
** significant bits of a prefix.
*/
int suffixnodesizes[5] = {1, 8, 16, 20, 0};	/* Must be 0 terminated */

uint32_t masks[33] = {
    0x00000000,						/* /0 ! */
    0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,	/* /1, /2, /3, /4 */
    0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,	/* /5, /6, /7, /8 */
    0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,	/* /9, /10, /11, /12 */
    0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,	/* /13, /14, /15, /16 */
    0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,	/* /17, /18, /19, /20 */
    0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,	/* /21, /22, /23, /24 */
    0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,	/* /25, /26, /27, /28 */
    0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff	/* /29, /30, /31, /32 */
};

cidrconf *cc = NULL;

/* Forward dclarations of internal functions */
static int	cidr_purge_sub(cidrnode**, void (*)(cidr_userdata_t*));
static int	cidr_expireall_sub(cidrnode*);
static int	cidr_insert_sub(cidrnode**, uint32_t, cidrnode*, void (*)(cidr_userdata_t*));
static int	cidr_remove_sub(cidrnode**, uint32_t, uint32_t, void (*)(cidr_userdata_t*));

/* cidr_init - Initialises all structures for the module
 *
 * Returns:
 *		 0 on success , new segment
 *		 1 on success , existing segment
 *		-1 on failure
 */

int cidr_init(const char* client_debug_name, int existing)
{
    unsigned int i;
    unsigned int j;
    suffixnode *snp;
    int	exseg=1; 
    
    /*
    ** attempt to attach to existing memory segment.
    */
    if ((cc = (cidrconf *)mpm_attach(CIDRMEMPATH)) == NULL) {
	/* attach failed, create a new one with the cc as the first reference */
	if (existing)
		return -1;
	exseg=0;
	if (mpm_init(CIDRMEMPATH, client_debug_name) == NULL)
	{
	  return -1;
	}
	mpm_wr_lock(3);
	cc = (cidrconf *)mpm_calloc(sizeof(*cc), 1); /* XXX - is that sizeof correct? */
	if(cc == NULL) {
	    errno = ENOMEM;
	    mpm_wr_unlock();
	    return -1;
	}
	cc->suffixes = NULL;
	cc->totalfiller = 0;
	/*
	** create a linked list of suffixnode structures.
	** where each suffixnode contains an array of every possible
	** route for that particular suffix length. 
	*/
	for(i = 0; suffixnodesizes[i] != 0; i++) {
	    snp = (suffixnode *)mpm_alloc(sizeof(suffixnode));
	    if(snp == (suffixnode *)NULL) {
		errno = ENOMEM;
		mpm_wr_unlock();
		return -1;
	    }
	    snp->suffix = suffixnodesizes[i];
	    snp->size = 1 << snp->suffix;
	    snp->base = (cidrnode **)mpm_alloc(sizeof(cidrnode *) * snp->size);
	    if(snp->base == (cidrnode **)NULL) {
		mpm_free(snp);
		errno = ENOMEM;
		mpm_wr_unlock();
		return -1;
	    }
	    for(j = 0; j < snp->size; j++)
		snp->base[j] = (cidrnode *)NULL;

	    /* insert the structure at the front of the list */
	    snp->next = cc->suffixes;
	    cc->suffixes = snp;
	}
	mpm_wr_unlock();
    }

    return exseg;
} /* cidr_init */


/*
** scans through the entire database looking for entries
** flagged as IP_EXPIRED.  These entries are deleted.
**
*/
int cidr_purge(void (*delete_userdata)(cidr_userdata_t*)) {
    int			counter;
    suffixnode*		snp;
    cidrnode*		cnp;
    unsigned int	i;
    unsigned int	maxidx;
    
    cidr_checktree();
    DFPRINTF((stdout, "cidr_purge()\n"));
    
    counter = 0;
    mpm_wr_lock(3);
    for(snp = cc->suffixes; snp != NULL; snp = snp->next) {
	maxidx = 1 << snp->suffix;
	for(i = 0; i < maxidx; i++)
	    if((cnp = snp->base[i]) != NULL)
		counter += cidr_purge_sub(&snp->base[i], delete_userdata);
    }
    mpm_wr_unlock();
    
    return counter;
}

int cidr_purge_sub(cidrnode **cnpp, void (*delete_userdata)(cidr_userdata_t*)) {
    cidrnode*		cnp;
    int			counter;
    
    counter = 0;
    cnp = *cnpp;
    
    if(cnp == NULL)
	return 0;
    
    if(cnp->left != NULL)
	counter += cidr_purge_sub(&(cnp->left), delete_userdata);
    
    if(cnp->right != NULL)
	counter += cidr_purge_sub(&(cnp->right), delete_userdata);
    
    if(cnp->flags & IP_FILLER) {
	if(cnp->flags & IP_EXPIRED) {
	    ; /* A filler should never be expired anyway */
	}
	
	cnp->flags &= ~(IP_EXPIRED);
	if(cnp->left != NULL && cnp->right != NULL)
	    return 0;
	
	if((*cnpp = cnp->left) == NULL) {
	    *cnpp = cnp->right;
	}
	
	mpm_free(cnp);
	return 0;
	
    } else {
	/*
	** not a FILLER node.
	*/
	if(!(cnp->flags & IP_EXPIRED))
	    return 0;
	
	/*
	** data both side, turn me into a filler.
	*/
	if(cnp->left != NULL && cnp->right != NULL) {
	    /* Expired but has two children. Covert to a filler node */
	    cnp->flags &= ~(IP_EXPIRED); 
	    cnp->flags |= IP_FILLER;
	    if(cnp->userdata.datap) {
		counter++;
		DFPRINTF((stdout, "(converting to a filler node) purging userdata (0x%p, %d) from %s/%d\n",
			  cnp->userdata.datap, cnp->userdata.size, ip_addr2str(cnp->prefix), cnp->suffix));
		if(delete_userdata == NULL) {
		    /* DFPRINTF((stdout, "using normal free mpm\n")); */
		    /* mpm_free(cnp->userdata.datap); */
		    ;
		} else {
		    DFPRINTF((stdout, "calling delete callback on %s/%d, 0x%p,%d\n",
			   ip_addr2str(cnp->userdata.prefix), cnp->userdata.length, cnp->userdata.datap, cnp->userdata.size));
		    delete_userdata(&cnp->userdata);
		}
		cnp->userdata.datap = NULL;
		cnp->userdata.size = 0;
		
	    }
	    return counter;
	}
	
	/*
	** Data on one side only.
	*/
	
	/*
	** unlink myself from above.
	*/
	if((*cnpp = cnp->left) == NULL) {
	    *cnpp = cnp->right;
	}
	
	DFPRINTF((stdout, "purging userdata (0x%p->0x%p,%d) from %s/%d\n",
	       &cnp->userdata, cnp->userdata.datap, cnp->userdata.size, ip_addr2str(cnp->prefix), cnp->suffix));
	counter++;
	if(delete_userdata == NULL) {
	    mpm_free(cnp->userdata.datap);
	} else {
	    DFPRINTF((stdout, "calling delete callback on %s/%d, 0x%p,%d\n",
		   ip_addr2str(cnp->userdata.prefix), cnp->userdata.length, cnp->userdata.datap, cnp->userdata.size));
	    delete_userdata(&cnp->userdata);
	}
	/*
	** These node is gone, but lets
	** nullify things just for good luck.
	*/
	cnp->userdata.datap = NULL;
	cnp->userdata.size = 0;
	mpm_free(cnp);
	return counter;
    }
} /* cidr_purge_sub */

/*
** scan through the entire database 
** and flag every entry as IP_EXPIRED
*/

int cidr_expireall(void) {
    int			counter;
    suffixnode*		snp;
    cidrnode*		cnp;
    unsigned int	i;
    unsigned int	maxidx;
    
    cidr_checktree();
    DFPRINTF((stdout, "cidr_expireall()\n"));
    counter = 0;
    mpm_wr_lock(3);
    for(snp = cc->suffixes; snp != NULL; snp = snp->next) {
	maxidx = 1 << snp->suffix;
	for(i = 0; i < maxidx; i++)
	    if((cnp = snp->base[i]) != NULL)
		counter += cidr_expireall_sub(cnp);
    }
    mpm_wr_unlock();
    return counter;
} /* cidr_expireall */


/*
** handle recursive expire for nodes.
*/
int cidr_expireall_sub(cidrnode *cnp) {
    int		counter;
    
    counter = 0;
    if(cnp == NULL)
	return 0;
    if(cnp->left != NULL)
	counter += cidr_expireall_sub(cnp->left);
    if(!(cnp->flags & IP_FILLER)) {
	counter++;
	cnp->flags |= IP_EXPIRED;
	DFPRINTF((stdout, "expiring %s/%d\n", ip_addr2str(cnp->prefix), cnp->suffix));
    }
    if(cnp->right != NULL) {
	counter += cidr_expireall_sub(cnp->right);
    }
    return counter;
} /* cidr_expireall_sub */


int cidr_update(cidr_userdata_t* userdata, void (*delete_userdata)(cidr_userdata_t*)) {
    
    int		retval;
    int		replaced;
    
    replaced = 0;
    
    
    retval = cidr_insert(userdata, delete_userdata);
    if(retval < 0) {
	if(errno == EEXIST) {
	    DFPRINTF((stdout, "-- insert failed, attemping delete then insert again\n"));
	    replaced = cidr_remove(userdata->prefix, userdata->length, delete_userdata);
	    retval = cidr_insert(userdata, delete_userdata);
	    if(retval < 0) {
		DFPRINTF((stdout, " *** bugger\n"));
	    }
	}
    }
    
    return retval;
}

/*
** insert userdata into the internal data structures, given a 
** prefix and a length.
**
*/


int cidr_insert(cidr_userdata_t* userdata,  void (*delete_userdata)(cidr_userdata_t*)) {
    uint32_t		prefix;
    uint32_t		suffix;
    suffixnode*		snp;
    cidrnode*		cnp;
    unsigned int	idx;
    
    /* We insert by looking up the sub-tree in the aggregated suffix tables. */
    /* cidr_printtree(); */
    DFPRINTF((stdout, "cidr_insert(%s/%d)\n", ip_addr2str(userdata->prefix), userdata->length));
    if(userdata == NULL) {
	errno = EINVAL;
	return -1;
    }
    
    prefix = userdata->prefix;
    suffix = userdata->length;
    
    /* 
    ** catch illegal lengths.
    */
    if (suffix > ADDRBITS) {
	errno = EDOM;
	return -1;
    }
    
    mpm_wr_lock(3);
    
    for(snp = cc->suffixes; snp != NULL; snp = snp->next) {
	if(suffix < snp->suffix)
	    continue;
	
	cnp = (cidrnode *)mpm_alloc(sizeof(cidrnode));
	if(cnp == NULL) {
	    errno = ENOMEM;
	    mpm_wr_unlock();
	    return -1;
	}
	cnp->prefix = prefix;
	cnp->suffix = suffix;
	cnp->flags = 0;
	cnp->left = NULL;
	cnp->right = NULL;
	
	/*
	** copy the user data information directly to this structure.
	*/
	
	memcpy(&cnp->userdata, userdata, sizeof(cidr_userdata_t));
	DFPRINTF((stdout, "inserting userdata at 0x%p (0x%p, %d) at %s/%d\n",
		  &cnp->userdata, cnp->userdata.datap, cnp->userdata.size, ip_addr2str(prefix), suffix));
	
	/*
	** recursively search for the insert point.
	*/
	/* shift the prefix down to become the index to the array */
	idx = prefix >> (ADDRBITS - snp->suffix);
	assert(idx < snp->size);
	if(snp->base[idx] == NULL) {
	    /*
	    ** if the list is empty, just insert it at the start.
	    */
	    snp->base[idx] = cnp;
	    DFPRINTF((stdout, "  inserting at an snp stub\n"));
	} else {
	    /*
	    ** otherwise insert down the tree somewhere.
	    */
	    DFPRINTF((stdout, "cidr_insert may do some damage to the following tree\n"));
	    cidr_checktree_sub(snp->base[idx], snp->base[idx]);
	    /* cidr_printtree_sub(snp->base[idx], 0); */
	    if(cidr_insert_sub(&(snp->base[idx]), snp->suffix, cnp, delete_userdata) < 0) {
		mpm_wr_unlock();
		return -1;
	    }
/* Darius - debugging, is slowing things down? */
/*	    cidr_checktree_sub(snp->base[idx], snp->base[idx]); */
	}
	
	break;
    }
    
    mpm_wr_unlock();
    
    return 0;
    
    
} /* cidr_insert */

static int cidr_insert_sub(cidrnode **cnpp, uint32_t suffix, cidrnode *incnp,  void (*delete_userdata)(cidr_userdata_t*)) {
    cidrnode*	cnp;
    cidrnode*	pfcnp;
    uint32_t	snp_suffix;
    
    snp_suffix = suffix;
    
    DFPRINTF((stdout, "cidr_insert_sub(%s/%d", ip_addr2str((*cnpp)->prefix), (*cnpp)->suffix));
    DFPRINTF((stdout, ", %s/%d\n", ip_addr2str(incnp->prefix), incnp->suffix));
    
    /* This will have already been write locked by cidr_insert */
    for(;;) {
	cnp = *cnpp;
	
	/* Check if we need to split on the current node */
	
	/* 
	** find the mask length at which the two nodes' prefix differs
	** by at least one bit.  Up until the bit before either of the length.
	*/
	DFPRINTF((stdout, "  walking until /%d or /%d\n", cnp->suffix, incnp->suffix));
	for(;suffix <= cnp->suffix && suffix <= incnp->suffix; suffix++) {
	    assert(suffix <= ADDRBITS);
	    DFPRINTF((stdout, "    incrementing suffix to %s/%d\n", ip_addr2str(cnp->prefix & masks[suffix]), suffix));
	    if((cnp->prefix & masks[suffix]) == (incnp->prefix & masks[suffix])) {
		DFPRINTF((stdout, "    %s/%d == ", ip_addr2str(cnp->prefix & masks[suffix]), suffix));
		DFPRINTF((stdout, "%s/%d\n", ip_addr2str(incnp->prefix & masks[suffix]), suffix));
		continue;
	    } else {
		DFPRINTF((stdout, "    %s/%d != ", ip_addr2str(cnp->prefix & masks[suffix]), suffix));
		DFPRINTF((stdout, "%s/%d\n", ip_addr2str(incnp->prefix & masks[suffix]), suffix));
	    }
	    
	    /* This just means the prefix is different */
	    /* We need to split on current suffix value */
	    
	    DFPRINTF((stdout, " path split on %s/%d ", ip_addr2str(incnp->prefix), incnp->suffix));
	    DFPRINTF((stdout, "against %s/%d\n", ip_addr2str(cnp->prefix), cnp->suffix));
	    pfcnp = (cidrnode *)mpm_alloc(sizeof(cidrnode));
	    if(pfcnp == NULL) {
		errno = ENOMEM;
		return -1;
	    }
	    
	    pfcnp->suffix = suffix - 1;	/* hmm.. */
	    assert(pfcnp->suffix <= ADDRBITS);
	    pfcnp->prefix = incnp->prefix & masks[pfcnp->suffix];
	    pfcnp->userdata.datap = NULL;
	    pfcnp->userdata.size = 0;
	    pfcnp->flags = 0;
	    pfcnp->flags |= IP_FILLER;
	    cc->totalfiller++;
	    DFPRINTF((stdout, " creating a filler node %s/%d\n", ip_addr2str(pfcnp->prefix), pfcnp->suffix));
		   
	    assert(suffix <= ADDRBITS);
	    DFPRINTF((stdout, " comparing inserted prefix (%s) with new filler prefix ", ip_addr2str((incnp->prefix & masks[suffix]))));
	    DFPRINTF((stdout, "(%s) with a mask on /%d\n", ip_addr2str(pfcnp->prefix), suffix));
	    if((incnp->prefix & masks[suffix]) == pfcnp->prefix) {
		DFPRINTF((stdout, " equal - new element goes left\n"));
		pfcnp->left = incnp;
		pfcnp->right = cnp;
	    } else {
		DFPRINTF((stdout, " not equal - new element goes right\n"));
		pfcnp->left = cnp;
		pfcnp->right = incnp;
	    }
	    *cnpp = pfcnp;
	    DFPRINTF((stdout, "going home\n"));
	    return 0;
	}
	
	DFPRINTF((stdout, "using %s/%d\n", ip_addr2str(cnp->prefix & masks[suffix]), suffix));
	/* Check if incnp is a parent */
	
	/* -jgc-comment-
	** if the inserting cnp (inncp) is a less significant
	** prefix the the existing cnp, then swap them around with the
	** new element as the parent.
	*/ 
	if(incnp->suffix < cnp->suffix) {
	    *cnpp = incnp;
	    DFPRINTF((stdout, "swapping the order of %s/%d and ", ip_addr2str(cnp->prefix), cnp->suffix));
	    DFPRINTF((stdout, "%s/%d\n", ip_addr2str(incnp->prefix), incnp->suffix));
	    /** XXX  -jgc-comment- this walks off the end by one for the case of a /32 */
	    assert(incnp->suffix + 1 <= ADDRBITS);
	    if((cnp->prefix & masks[incnp->suffix + 1]) == incnp->prefix)
		incnp->left = cnp;
	    else
		incnp->right = cnp;
	    return 0;
	}
	
	/*
	** length is equal.
	*/ 
	if(incnp->suffix == cnp->suffix) {
	    DFPRINTF((stdout, "we're at the appropriate depth /%d - we have to do something here\n", cnp->suffix));
	    DFPRINTF((stdout, "Found a peer %s/%d", ip_addr2str(cnp->prefix), cnp->suffix));
	    DFPRINTF((stdout, " for %s/%d", ip_addr2str(incnp->prefix), incnp->suffix));
	    DFPRINTF((stdout, " (the common prefix = %s/%d)\n", ip_addr2str(incnp->prefix & masks[snp_suffix]), snp_suffix));
	    
	    if(incnp->prefix == cnp->prefix) {	/* match! */
		if(cnp->flags & IP_FILLER) { 
		    /* Matched a filler - make non-filler */
		    
		    DFPRINTF((stdout, "Made %s/%d a non-filler on", ip_addr2str(cnp->prefix), cnp->suffix));
		    DFPRINTF((stdout, " %s/%d\n", ip_addr2str(incnp->prefix), incnp->suffix));
		    
		    cnp->flags &= ~(IP_FILLER);
		    cc->totalfiller--;
		    memcpy(&cnp->userdata, &incnp->userdata, sizeof(cidr_userdata_t));
		    mpm_free(incnp);
		    return 0;
		} else if (cnp->flags & IP_EXPIRED) {
		    /*
		    ** the exact matched was already here, but it was
		    ** expired.  Delete the old user data and replace
		    ** it, leave the rest of the node in place.
		    */
		    DFPRINTF((stdout, "replacing expired %s/%d\n", ip_addr2str(cnp->prefix), cnp->suffix));
		    
		    cnp->flags &= ~(IP_EXPIRED);
		    if(cnp->userdata.datap) {
			if(delete_userdata) {
			    delete_userdata(&cnp->userdata);
			    cnp->userdata.datap = NULL;
			    cnp->userdata.size = 0;
			}
		    }
		    memcpy(&cnp->userdata, &incnp->userdata, sizeof(cidr_userdata_t));
		    mpm_free(incnp);
		    return 0;
		} else {
		    mpm_free(incnp);
		    DFPRINTF((stdout, "ERROR - duplicate for %s/%d\n", ip_addr2str(cnp->prefix), cnp->suffix));
		    errno = EEXIST;
		    return -1;
		}
	    }
	    
	    /* Create a parent filler node */
	    
	    pfcnp = (cidrnode *)mpm_alloc(sizeof(cidrnode));
	    if(pfcnp == NULL) {
		errno = ENOMEM;
		return -1;
	    }
	    DFPRINTF((stdout, "creating a filler above us\n"));
	    pfcnp->suffix = cnp->suffix - 1;
	    assert(pfcnp->suffix <= ADDRBITS);
	    pfcnp->prefix = cnp->prefix & masks[pfcnp->suffix];
	    pfcnp->userdata.datap = NULL;
	    pfcnp->userdata.size = 0;
	    pfcnp->flags = 0;
	    pfcnp->flags |= IP_FILLER;
	    cc->totalfiller++;

	    if(incnp->prefix < cnp->prefix) { /* left */
		pfcnp->left = incnp;
		pfcnp->right = cnp;
	    } else {	/* right */
		pfcnp->left = cnp;
		pfcnp->right = incnp;
	    }
	    *cnpp = pfcnp;
	    return 0;
	}
	
	/* Otherwise incnp is a child further down somewhere */
	/* Branch left or right depending on the bit value of the next bit */
	/* after this node's suffix */
	assert(cnp->suffix + 1 <= ADDRBITS);
	if((incnp->prefix & masks[cnp->suffix + 1]) == cnp->prefix) { /* left */
	    DFPRINTF((stdout, " going left\n"));
	    if(cnp->left == NULL) {
		cnp->left = incnp;
		return 0;
	    }
	    cnpp = &(cnp->left);
	} else {	/* right */
	    DFPRINTF((stdout, " going right\n"));
	    if(cnp->right == NULL) {
		cnp->right = incnp;
		return 0;
	    }
	    cnpp = &(cnp->right);
	}
    }
} /* cidr_insert_sub */


int cidr_remove(uint32_t prefix, uint32_t suffix, void (*delete_userdata)(cidr_userdata_t*)) {
    suffixnode*		snp;
    uint32_t		idx;
    int			retcode;
    DFPRINTF((stdout, "------------\n"));
    DFPRINTF((stdout, "cidr_remove(%s/%d)\n", ip_addr2str(prefix), suffix));
    
    mpm_wr_lock(3);
    /*
    ** find the appropriate suffix node.
    */
    for(snp = cc->suffixes; snp != NULL && suffix < snp->suffix; snp = snp->next);
    DFPRINTF((stdout, "scanning suffix list on /%d\n", snp->suffix));
    if(snp == NULL) {
	errno = ENOENT;
	mpm_wr_unlock();
	return -1;
    }
    
    idx = prefix >> (ADDRBITS - snp->suffix);
    DFPRINTF((stdout, "using index of %d (0x%X)\n", idx, idx));
    assert(idx < snp->size);
    if(snp->base[idx] == NULL) {
	DFPRINTF((stdout, "base for /%d was NULL\n", snp->suffix));
	errno = ENOENT;
	mpm_wr_unlock();
	return -1;
    }
    DFPRINTF((stdout, "here's a view of the tree hanging off %s/%d before we start\n", ip_addr2str(prefix & masks[snp->suffix]), snp->suffix));
    cidr_checktree_sub(snp->base[idx], snp->base[idx]);
    /* cidr_printtree_sub(snp->base[idx], 0); */
    DFPRINTF((stdout, "attempting recursive remove from %s/%d downwards\n", ip_addr2str(prefix & masks[snp->suffix]), snp->suffix));
    retcode = cidr_remove_sub(&(snp->base[idx]), prefix, suffix, delete_userdata);
    cidr_checktree_sub(snp->base[idx], snp->base[idx]);
    mpm_wr_unlock();
    
    return retcode;
} /* cidr_remove */

static int cidr_remove_sub(cidrnode **cnpp, uint32_t prefix, uint32_t suffix, void (*delete_userdata)(cidr_userdata_t*)) {
    cidrnode	*cnp;
    int		retcode;
    
    /* This will have already been write locked by cidr_remove() */
    if(cnpp == NULL || *cnpp == NULL) {
	errno = ENOENT;
	return -1;
    }
    
    cnp = *cnpp;
    
    if(suffix < cnp->suffix) {
	DFPRINTF((stdout, "   we've exceeded our suffix range of /%d, no point checking further\n", suffix));
	errno = ENOENT;
	return -1;
    }
    
    /*
    ** exact match.
    */
    DFPRINTF((stdout, "   comparing %s/%d with ", ip_addr2str(prefix), suffix));
    DFPRINTF((stdout, "with %s/%d\n", ip_addr2str(cnp->prefix), cnp->suffix));
    
    if(prefix == cnp->prefix && suffix == cnp->suffix) {
	DFPRINTF((stdout, "   found!\n"));
	if(cnp->left == NULL) {
	    if(cnp->right == NULL) {
		DFPRINTF((stdout, "   right was null\n"));
		*cnpp = NULL;
	    } else {
		*cnpp = cnp->right;
	    }
	} else {
	    if(cnp->right == NULL) {
		DFPRINTF((stdout, "   right was null\n"));
		*cnpp = cnp->left;
	    } else {
		DFPRINTF((stdout, "   we now become a filler node - deleting user data.\n"));
		if(cnp->userdata.datap) {
		    if(delete_userdata) {
			delete_userdata(&cnp->userdata);
			cnp->userdata.datap = NULL;
			cnp->userdata.size = 0;
		    }
		}
		cnp->flags |= IP_FILLER;
		return 0;
	    }
	}
	
	if(cnp->userdata.datap) {
	    DFPRINTF((stdout, "   deleting user data\n"));
	    if(delete_userdata) {
		delete_userdata(&cnp->userdata);
		cnp->userdata.datap = NULL;
		cnp->userdata.size = 0;
	    }
	}
	mpm_free(cnp);
	return 0;
    }
    
    /*
    ** otherwise, keep searching.
    */

    /* harryr - /32's routes are a problem here. don't know why
       this assert is like this */
    assert(cnp->suffix + 1 <= ADDRBITS);
    DFPRINTF((stdout, "comparing %s/%d with ", ip_addr2str(prefix & masks[cnp->suffix + 1]), cnp->suffix + 1));
    DFPRINTF((stdout, " %s/%d\n", ip_addr2str(cnp->prefix), cnp->suffix));
    if((prefix & masks[cnp->suffix + 1]) == cnp->prefix) {
	DFPRINTF((stdout, "   going left\n"));
	retcode = cidr_remove_sub(&(cnp->left), prefix, suffix, delete_userdata);
    } else {
	DFPRINTF((stdout, "   going right\n"));
	retcode = cidr_remove_sub(&(cnp->right), prefix, suffix, delete_userdata);
    }
    
    /*
    ** ah - this is a tidy up one left up after the possible delete
    ** one level below.
    */
    if((cnp->flags & IP_FILLER) && cnp->left == NULL) {
	DFPRINTF((stdout, "   pass down with NULL left\n"));
	*cnpp = cnp->right;
	mpm_free(cnp);		/* XXX - what's this doing? */
	return retcode;
    }
    if((cnp->flags & IP_FILLER) && cnp->right == NULL) {
	DFPRINTF((stdout, "   pass down with NULL right\n"));
	*cnpp = cnp->left;
	mpm_free(cnp);		/* XXX - what's this doing? */
	return retcode;
    }
    DFPRINTF((stdout, ".\n"));
    return retcode;
} /* cidr_remove_sub */



/*
** return -1 == failure.
** return 0 == sucess.
**
** data is passed back in userdata.
** a match is attempted on the values in userdata
**
** The returned data may have a different prefix and length
** depending on the match found.
**
** The returned data is a copy of the real data.. The copy if complete
** while the real shared data is locked, to prevent corruption.
**
*/
int cidr_lookup(cidr_userdata_t* userdata) {
    uint32_t		prefix;
    uint32_t		suffix;
    suffixnode*		snp;
    cidrnode*		cnp;
    cidrnode*		matchp = NULL;
    unsigned int	idx;
    int			compares = 0;
    
    if(userdata == NULL) {
	errno = EINVAL;
	return -1;
    }
    
    prefix = userdata->prefix;
    suffix = userdata->length;
    
    if(suffix > ADDRBITS) {
	errno = EDOM;
	return -1;
    }
    
    mpm_rd_lock(3);
    /*
    ** find the appropriate suffix block in the list.
    */
    for(snp = cc->suffixes; snp != NULL; snp = snp->next) {
	compares++;
	if(suffix < snp->suffix)
	    continue;
	
	compares++;
	/*
	** shift the valid bits down for form an index.
	*/
	idx = prefix >> (ADDRBITS - snp->suffix);
	assert(idx < snp->size);
	cnp = snp->base[idx];
	if(cnp == NULL) { 
	    continue;
	}
	
	/*
	** non-recursive tree walk to find the best match network.
	**
	*/ 
	while(cnp != NULL) {
	    compares++;
	    if(suffix < cnp->suffix)
		break;
	    compares++;
	    assert(cnp->suffix <= ADDRBITS);
	    if((prefix & masks[cnp->suffix]) == cnp->prefix)
		if(!(cnp->flags & IP_FILLER))	/* match */
		    matchp = cnp;
	    compares++;
	    if(cnp->suffix + 1 > ADDRBITS) {
		cnp = NULL;
	    } else {
		if((prefix & masks[cnp->suffix + 1]) == cnp->prefix) {
		    cnp = cnp->left;
		} else {
		    cnp = cnp->right;
		}
	    }
	}
	if(matchp)
	    break;
    }
    
/*
    DFPRINTF((stdout, "Compares = %d\n", compares));
*/ 
    if(matchp == NULL) {
	userdata->prefix = 0;
	userdata->length = 0;
	userdata->datap = NULL;
	userdata->size = 0;
	mpm_rd_unlock();
	return -1;
    }
    
    if(matchp->userdata.prefix != matchp->prefix || matchp->userdata.length != matchp->suffix) {
	fprintf(stdout, "ERROR in lookup - userdata values didn't match\n");
	mpm_rd_unlock();
	errno = EIO;
	return -1;
    }

   /*
    ** userdata should already contain the correct values
    ** check anyway..
    */
    if(userdata->size < matchp->userdata.size) {
	fprintf(stdout, "ERROR in lookup - incorrect size of data within userdata\n");
	mpm_rd_unlock();
	errno = EIO;
	return -1;
    }
    
    /*
    ** copy the userdata out.
    */
    memcpy(userdata->datap, matchp->userdata.datap, matchp->userdata.size);
    userdata->length = matchp->userdata.length;
    userdata->prefix = matchp->userdata.prefix;
    
    mpm_rd_unlock();
    return 0;
} /* cidr_lookup */


/*
 * cidr_watchdog_tick
 * Update the watchdog timer. Called from within providers regularly
 */
void cidr_watchdog_tick() {
    cc->watchdog_time = time(NULL);
}

/*
 * cidr_watchdog_ok
 * Check whether the watchdog timer is so late that there is a problem
 * with the provider.
 */
int cidr_watchdog_ok() {
    return (cc->watchdog_time > time(NULL) - CIDR_WATCHDOG_LATE_TIME);
}


void cidr_printtree(void) {
    suffixnode *snp;
    cidrnode *cnp;
    unsigned int i, maxidx;
    
    printf("\n----\ncidr_printtree()\n");
    mpm_wr_lock(3);
    for(snp = cc->suffixes; snp != NULL; snp = snp->next) {
	maxidx = 1 << snp->suffix;
	for(i = 0; i < maxidx; i++)
	    if((cnp = snp->base[i]) != NULL)
		cidr_printtree_sub(cnp, 0);
    }
    mpm_wr_unlock();
    printf("end\n-----\n");
}


/*
** handle recursive expire for nodes.
*/
void cidr_printtree_sub(cidrnode *cnp, int level) {
    int i;
    
    if(cnp == NULL) {
	printf("NULL\n");
	return;
    }
    
    printf("%s/%d (0x%p, %d)", ip_addr2str(cnp->prefix), cnp->suffix, cnp->userdata.datap, cnp->userdata.size);
    if(cnp->flags & IP_FILLER) {
	printf("(FILLER)");
    }
    if(cnp->flags & IP_EXPIRED) {
    	printf(" ###################### (EXPIRED)");
    }
    printf("\n");
    if(cnp->left) {
	for(i=0; i <= level ; i++) {
	    printf(".   ");
	}
	printf("left = ");
	cidr_printtree_sub(cnp->left, level+1);
    }
    if(cnp->right) {
	for(i=0; i <= level ; i++) {
	    printf(".   ");
	}
	printf("right = ");
	cidr_printtree_sub(cnp->right, level+1);
    }
}

/*
** This is a transient debug function that gets used to find the "bug
** of the day"
**
** today it checks for malformed trees, with sub-nodes which are on the wrong side.
**
*/
void cidr_checktree(void) {
    suffixnode *snp;
    cidrnode *cnp;
    unsigned int i, maxidx;
    
    mpm_wr_lock(3);
    for(snp = cc->suffixes; snp != NULL; snp = snp->next) {
	maxidx = 1 << snp->suffix;
	for(i = 0; i < maxidx; i++) {
	    if((cnp = snp->base[i]) != NULL) {
		cidr_checktree_sub(cnp, cnp);
	    }
	}
    }
    mpm_wr_unlock();
}

/*
** check the left and right trees are correct all the way down.
*/
void cidr_checktree_split(cidrnode* base, cidrnode* cnp, uint32_t prefix, uint32_t length) {
    cidrnode*		left;
    cidrnode*		right;
    
    if(cnp == NULL) {
	return;
    }
    /*
    printf("\tcidr_chectree_split(%s/%d",  ip_addr2str(base->prefix), base->suffix); 
    printf(", %s/%d",  ip_addr2str(cnp->prefix), cnp->suffix);
    printf(", %s/%d\n",  ip_addr2str(prefix), length);
    */
    {
	uint32_t mask;
	
	mask = ~((1 << (32 - length)) - 1);
	
	if(masks[length] != mask) {
	    printf("MASK ERROR - masks[%d] = 0x%X != 0x%X\n", length, masks[length], mask);
	    cidr_printtree_sub(base, 0);
	    abort();
	}
    }
    
    left = cnp->left;
    right = cnp->right;
    
    if(cnp->suffix == 32) {
	if(cnp->left || cnp->right) {
	    printf("TREE ERROR - %s/%d has", ip_addr2str(cnp->prefix), cnp->suffix);
	    printf(" /32 has children\n");
	    cidr_printtree_sub(base, 0);
	    abort();
	}
    }
    
    if(cnp->suffix > 32) {
	    printf("TREE ERROR - %s/%d has", ip_addr2str(cnp->prefix), cnp->suffix);
	    cidr_printtree_sub(base, 0);
	    abort();
    }	
    
    if((cnp->prefix) & (~masks[cnp->suffix])) {
	printf("TREE ERROR - %s/%d has", ip_addr2str(cnp->prefix), cnp->suffix);
	printf(" non-zero bits past mask boundary\n");
	cidr_printtree_sub(base, 0);
	abort();
    }
    
    if((cnp->prefix & masks[length]) != (prefix & masks[length])) {
	printf("TREE ERROR - %s/%d has", ip_addr2str(cnp->prefix), cnp->suffix);
	printf(" has left/right brokenness.\n");
	cidr_printtree_sub(base, 0);
	abort();
    }
    
    if(left) {
	cidr_checktree_split(base, cnp->left, prefix, length);
	cidr_checktree_split(base, cnp->left, cnp->prefix, cnp->suffix + 1);
    }
    
    if(right) {
	uint32_t	mask;
	uint32_t	bit;
	
	mask = masks[cnp->suffix + 1];
	bit = 1 << (32 - (cnp->suffix + 1));
	
	cidr_checktree_split(base, cnp->right, prefix, length);
	cidr_checktree_split(base, cnp->right, (cnp->prefix | bit) & mask, cnp->suffix + 1);
    }
}

/*
** !! this is temporary stuff for BGP specific checking to help debug.
**
*/ 
void cidr_checktree_sub(cidrnode* base, cidrnode *cnp) {
    cidrnode* left;
    cidrnode* right;
    
    if(cnp == NULL) {
	return;
    }
    
    cidr_checktree_split(base, cnp, cnp->prefix, cnp->suffix);
}


/*
** export data that matches a route and below.
*/
int cidr_lookup_tree(uint32_t prefix, uint32_t suffix,  void (*export_data)(cidr_userdata_t*)) {
    suffixnode*		snp;
    cidrnode*		cnp;
    unsigned int	idx;
    unsigned int	maxidx;
    
    if(export_data == NULL) {
	errno = EINVAL;
	return -1;
    }
    
    if(suffix > ADDRBITS) {
	errno = EDOM;
	return -1;
    }
    
    mpm_rd_lock(3);
    /*
    ** find the appropriate suffix block in the list.
    */
    for(snp = cc->suffixes; snp != NULL; snp = snp->next) {
	uint32_t small;
	
	maxidx = 1 << snp->suffix;
	for(idx = 0; idx < maxidx; idx++) {
	    if((cnp = snp->base[idx]) != NULL) {
		uint32_t	snp_prefix;
		
		snp_prefix = idx << (ADDRBITS - snp->suffix);
		if(snp->suffix < suffix) {
		    small = snp->suffix;
		} else {
		    small = suffix;
		}
		if((prefix & masks[small]) == (snp_prefix & masks[small])) {
		    /* printf("%s/%d\n", ip_addr2str(snp_prefix), snp->suffix); */
		    cidr_lookup_tree_sub(snp->base[idx], prefix, suffix, small, export_data);
		} else {
		    /* printf("skipped %s/%d, using mask of 0x%X - ", ip_addr2str(snp_prefix), snp->suffix, masks[snp->suffix]); */
		    /* printf(" on %s/%d\n", ip_addr2str(prefix & masks[snp->suffix]), snp->suffix); */
		}
	    }
	}
    }
    mpm_rd_unlock();
    return 0;
}


int cidr_lookup_tree_sub(cidrnode* cnp, uint32_t prefix, uint32_t suffix,  uint32_t snp_suffix, void (*export_data)(cidr_userdata_t*)) {
    /*
    ** non-recursive tree walk to find the best match network.
    **
    */
    if (cnp == NULL) {
	return -1;
    }
    
    /*
    ** compare the real masks..
    */
    if((cnp->prefix & masks[suffix]) == (prefix & masks[suffix])) {
	if(!(cnp->flags & IP_FILLER)) {
	    export_data(&cnp->userdata);
	}
    } else {
	/* printf("walking %s/%d, using mask of 0x%X - ", ip_addr2str(cnp->prefix), cnp->suffix, masks[cnp->suffix]); */
	/* printf(" on %s/%d\n", ip_addr2str(prefix & masks[cnp->suffix]), cnp->suffix); */
	;
    }
    
    if((prefix & masks[snp_suffix]) == (cnp->prefix & masks[snp_suffix])) {
	cidr_lookup_tree_sub(cnp->left, prefix, suffix, snp_suffix, export_data);
	cidr_lookup_tree_sub(cnp->right, prefix, suffix, snp_suffix, export_data);	    
    }
    return 0;
}
