#ifndef MPMALLOC_H
#define MPMALLOC_H

#include	<synch.h>

#define MAXMEM		0x20000000
#define MAPSTART	(caddr_t)0x20000000
#define MAXBITS		64
#define	SUBSPLITS	2
#define	NUMBUCKETS	(MAXBITS * SUBSPLITS)
#define	SYSPAGESIZE	8192
#define	FGROWSIZE	1048576	/* Grow file by at least this amount */
#define	MINALLOCSIZE	16
#define	DEFAULTNUMCHUNKS (SYSPAGESIZE / MINALLOCSIZE)
#define	MAXDEFAULTNUMPAGES	2
#define MAXDEFAULTSPACE	(SYSPAGESIZE * MAXDEFAULTNUMPAGES)

#define BITSET(a, b)	(((a[b/8]) >> (7-(b%8))) & 0x1)
#define SETBIT(a, b)	((a[b/8]) |= (1<<(7-(b%8))))
#define CLRBIT(a, b)	((a[b/8]) &= (~(1<<(7-(b%8)))))

/* Allocate batches of free page indicators.  Each indicator points to a free */
/* page of memory.  Multiple pages of free memory are grouped together with a */
/* pointer to that set of free pages. A numpages value of 0 means that this */
/* freeptr is currently not in use. */

typedef struct freeptr {
    char  *page;
    struct freeptr *next;	/* An ordered doubly linked list of free */
    struct freeptr *prev;	/* pages, ordered by base page addresses */
    int   numpages;
} freeptr;

/* A freepage has a set of freeptrs.  Once a freepage gets to 0 free freeptrs */
/* then it's moved to a list of freepages that have no freeptrs. If a */
/* freepage reached all of its freeptrs being free, then the freepage itself */
/* becomes freed if the freepage free list already contains a freepage */

#define	FREEPAGEOVERHEAD	(sizeof(void *) * 2 + sizeof(int))
#define FREEPTRSPERFREEPAGE	((SYSPAGESIZE-FREEPAGEOVERHEAD)/sizeof(freeptr))

typedef struct freepage {
    struct freepage *next;
    struct freepage *prev;
    int    freefreeptrs;
    freeptr freeptrs[FREEPTRSPERFREEPAGE];
} freepage;

/* The `malptr' structure represents a bitmap of the allocated chunks of */
/* a page.  The `page' base pointer is set to NULL if no pages are allocated */
/* to the malptr structure */

typedef struct malptr {
    char   *page;		/* Pointer to base of allocated page(s) */
    struct malptr *next;	/* A linked list of like malptr's in a bucket */
    struct malptr *left;	/* An top-down splay tree of malptrs ordered */
    struct malptr *right;	/* by base page addresses. */
    short  numchunks;		/* Number of chunks in bitmap */
    short  freechunks;		/* Number of unused chunks in bitmap */
    int    chunksize;		/* Size of each chunk */
    int    numpages;
    unsigned char   use[DEFAULTNUMCHUNKS/8];   /* Bitmap of use of each chunk */
} malptr;

#define	MALPAGEOVERHEAD		(sizeof(void *) * 2 + sizeof(int))
#define	MALPTRSPERMALPAGE	((SYSPAGESIZE-MALPAGEOVERHEAD)/sizeof(malptr))

typedef struct malpage {
    struct malpage *next;	/* A linked list of part-full (used) malpages */
    int    freemalptrs;		/* Number of free malptr's in this malpage */
    malptr malptrs[MALPTRSPERMALPAGE];
} malpage;

/* Ok.  Have pages of malptr structures.  Each malptr points to a page of */
/* free memory and has a bitmaps stating the usage of each chunk of that */
/* memory. */

typedef struct mpmstate {
    int seq;			/* Initialisation sequence */
    sema_t mutex;		/* Mutex semaphore */
    sema_t rd;			/* Reader wait semaphore */
    sema_t wr;			/* Writer wait semaphore */
    uint wr_wait;		/* Number of waiting writers */
    uint rd_wait;		/* Number of waiting readers */
    uint wr_act;		/* Number of active writers */
    uint rd_act;		/* Number of active readers */
    time_t brkrdlocktime;	/* Time to break last read lock - 0 if none */
    time_t brkwrlocktime;	/* Time to break last write lock - 0 if none */
    time_t breaker_tick;	/* Last time lock breaker process was here */
    pid_t breaker_pid;		/* Pid of breaker process - detect multiples */

    size_t msize;		/* Size of memory mapping */
    size_t fsize;		/* File size (only grows) */
    void *firstblock;		/* First allocated block */
    freeptr *freepagelist;	/* List of unused pages */
    freepage *freefreepages;	/* List of unused free page indicies */
    freepage *usedfreepages;	/* List of partially used free page indicies */
    freepage *fullfreepages;	/* List of full free page indicies */
    malpage *used_malpages;	/* Tree of used allocated page indicies */
    malptr *rootmalptr;		/* */
    malptr *used_malptrs[NUMBUCKETS]; /* Trees of like size bucket indicies */
} mpmstate;

/* We ned to be able to run without shared memory sometimes */
#ifdef DONT_USE_MPM
#include        <stdlib.h>
#define mpm_init(x)         1
#define mpm_attach(x)       NULL
#define mpm_calloc(x,y)     calloc(x, y)
#define mpm_alloc(x)        malloc(x)
#define mpm_free(x)         free(x)
#define mpm_wr_lock(x)      
#define mpm_wr_unlock()     
#define mpm_rd_lock(x)      
#define mpm_rd_unlock()     

#else

void	*mpm_init(const char *, const char*);
void	*mpm_attach(const char *);
void	*mpm_attach_clear_sem(const char *);
void	*mpm_alloc(size_t);
void	*mpm_calloc(size_t, size_t);
void	mpm_free(void *);
char	*mpm_strdup(const char *);
void	*mpm_align(size_t, size_t);
void	*mpm_realloc(void *, size_t);
void	*mpm_valloc(size_t);
size_t	mpm_allocsize(void *);
int	mpm_rd_lock(int);
int	mpm_wr_lock(int);
int	mpm_rd_unlock();
int	mpm_wr_unlock();
extern void	mpm_debug_init(const char*);
extern void	mpm_debug_close(void);

#endif

#endif
