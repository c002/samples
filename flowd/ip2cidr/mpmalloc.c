/*	Stew's crappy mmap-based malloc with advisory locking library */
/* 	$Id: mpmalloc.c,v 1.1.1.1 2004/05/29 09:06:55 harry Exp $	 */

/*
** --jgc-- added some debug routines to checked suspected 
** locking problems..
**
*/

/* #define DARIUS_DEBUG */

static char vcid[] = "$Id: mpmalloc.c,v 1.1.1.1 2004/05/29 09:06:55 harry Exp $";

#include	<sys/types.h>
#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<strings.h>
#include	<errno.h>
#include	<stdio.h>
#include	<stdarg.h>
#include	<time.h>

#include	"mpmalloc.h"

#ifndef DONT_USE_MPM
static int rd_lock_cnt = 0;	/* Count how many times this process using */
static int wr_lock_cnt = 0;	/* this library has locked the memory mapping */
static int mpmseq = 0;
static char mpmpath[MAXPATHLEN];
static mpmstate *state = NULL;
static caddr_t pa = NULL;	/* Base of memory mapping */
static caddr_t ba = NULL;	/* Base of usable memory */
static size_t mapsize = 0;	/* Instance account of size of mapping */
static FILE*	debug_f = NULL;
static char*	client_name = NULL;

static size_t	mpm_grow_file_and_map(const char *, size_t);
static void	*mpm_sbrk(const char *, int);
static void	mpm_init_sem();
static int	mpm_init_state();
static void	mpm_lock_breaker();

static freepage *claim_freepage();
static void	unlink_freepage(freepage *);
static void	link_freepage(freepage *);
static void	relink_freepage(freepage *);
static freeptr	*claim_freeptr();
static void	free_freeptr(freeptr *);
static void	put_freeptr(freeptr *);
static void	free_pages(char *, int);
static char	*claim_pages(int);
static int	get_bucketnum(int);
static int	get_chunksize(int);
static void	link_malpage(malpage *);
static void	unlink_malpage(malpage *);
static malpage	*get_malpage();
static void	link_malptr(malptr *, int);
static void	unlink_malptr(malptr *, int);
static char	*get_memory(int, int);
static void	free_memory(char *);
static void	mpm_check_remap();
static int	mpm_rd_sem_lock(int);
static int	mpm_wr_sem_lock(int);
static int	mpm_rd_sem_unlock();
static int	mpm_wr_sem_unlock();
#ifdef DEBUG_MALLOC
static void	splay_check(malptr *, char *, char *);
#endif
static void     splay_insert(malptr **, malptr *);
static malptr	*splay_find(malptr *, char *);
static malptr	**splay_find_splay(malptr **, char *);
static malptr	*splay_remove(malptr **, char *);
static void	mpm_debug(const char *, ...);

size_t
mpm_grow_file_and_map(const char *path, size_t newsize)
{
    int fd;
    char bytebuf[1];
    struct stat statbuf;

    if((fd = open(path, O_RDWR | O_CREAT, 0600)) < 0)
	return -1;
    if(newsize > 0) {
	bytebuf[0] = 0; /* The read may fail */

	if(lseek(fd, (off_t)(newsize - 1), SEEK_SET) < 0) {
	    close(fd);
	    return -1;
	}
	if(read(fd, bytebuf, 1) < 0) {
	    close(fd);
	    return -1;
	}
	if(lseek(fd, (off_t)(newsize - 1), SEEK_SET) < 0) {
	    close(fd);
	    return -1;
	}
	if(write(fd, bytebuf, 1) < 0) {
	    close(fd);
	    return -1;
	}
    }
    if(fstat(fd, &statbuf) < 0) {
	close(fd);
	return -1;
    }
    newsize = statbuf.st_size;

    pa = mmap(MAPSTART, newsize, (PROT_READ | PROT_WRITE), (MAP_SHARED | MAP_FIXED), fd, 0);

    close(fd);

    if(newsize == 0)
	return 0;
    if(pa == MAP_FAILED) {
	return -1;
    }
    mapsize = newsize;
    return newsize;
} /* mpm_grow_file_and_map */


/* This function differs from sbrk() in a number of ways */
/* It returns the new break point (rather than the old) */
/* The new break point may be larger than the amount of memory */
/* that was asked for due to alignment restrictions. */

static void *
mpm_sbrk(const char *path, int incr)
{
    size_t size;
    int grow;
    char *oldbrk = NULL;

    errno = 0;

    if(state != NULL) {		/* A mapping already exists */
	if(incr == 0) {
	    oldbrk = (char *)pa + state->msize;
	    return (void *)oldbrk;
	}

	oldbrk = (char *)pa + state->msize;
	size = state->msize + incr;
	state->msize = size;

	if(size < SYSPAGESIZE)
	    size = SYSPAGESIZE;

	if(size > state->fsize) {
	    grow = (size - state->fsize);
	    if(grow < FGROWSIZE)
		grow = FGROWSIZE;
	    else
		grow = ((grow / FGROWSIZE) + 1) * FGROWSIZE;
	    size = mpm_grow_file_and_map(path, state->fsize + grow);
	    if(size < 0) {
		errno = ENOMEM;
		return NULL;
	    }
	    state->fsize = size;
	}

    } else {
	size = mpm_grow_file_and_map(path, 0);	/* Attempt to attach */
	if(size == 0) {			/* New file - grow it */
	    if((size = mpm_grow_file_and_map(path, FGROWSIZE)) > 0) {
		state = (mpmstate *)pa;
		state->fsize = size;
		state->msize = size;
		ba = (void *)(((char *)pa) + SYSPAGESIZE);
		oldbrk = ba;
	    }
	  }
	else if (size == -1)
	  {
	    errno = ENOMEM;
	    return NULL;
	} else {
	    state = (mpmstate *)pa;
	    oldbrk = (char *)pa + state->msize;
	}
    }

    return (char *)oldbrk;
} /* mpm_sbrk */


static void
mpm_init_sem()
{
    mpm_debug("Initialising Semaphores\n");
    sema_init(&(state->mutex), 1, USYNC_PROCESS, NULL);
    sema_init(&(state->rd), 0, USYNC_PROCESS, NULL);
    sema_init(&(state->wr), 0, USYNC_PROCESS, NULL);
    sema_wait(&(state->mutex));
    state->wr_wait = 0;
    state->rd_wait = 0;
    state->wr_act = 0;
    state->rd_act = 0;
    state->brkrdlocktime = 0;
    state->brkwrlocktime = 0;
    state->breaker_tick = 0;
    state->breaker_pid = 0;
    sema_post(&(state->mutex));
} /* mpm_init_sem */


static int
mpm_init_state()
{
    int i;
    int seq;

    mpm_init_sem();
    sema_wait(&(state->mutex));
    seq = ++(state->seq);
    state->msize = SYSPAGESIZE;
    state->firstblock = NULL;
    state->freepagelist = NULL;
    state->freefreepages = NULL;
    state->usedfreepages = NULL;
    state->fullfreepages = NULL;
    state->used_malpages = NULL;
    state->rootmalptr = NULL;
    for(i = 0; i < NUMBUCKETS; i++)
	state->used_malptrs[i] = NULL;

    claim_freepage();
    sema_post(&(state->mutex));
    return seq;
} /* mpm_init_state */


/* This function will create AND overwrite all structures associated with */
/* the mpmfilepath memory file.  If the memory file already exists and is */
/* already initialised, this function will acquire a write lock first before */
/* reinitialising the structures. */
/* If the file could not be attached to, returns NULL */

void *
mpm_init(const char *mpmfilepath, const char* client_debug_name)
{
    char *mem;

    strncpy(mpmpath, mpmfilepath, MAXPATHLEN);
    mpmpath[MAXPATHLEN - 1] = 0;

    state = NULL;
    mem = (char *)mpm_sbrk(mpmpath, 0);	/* Handles alignment for us */
    if (mem)
      {
	mpmseq = mpm_init_state();
	return mem;
      }
    else
      return NULL;
} /* malloc_init */


/* This function attempts to attach to an already existing persistent */
/* memory mapping */

void *
mpm_attach(const char *mpmfilepath)
{
    char *mem;
    struct stat statbuf;
    char *addr;

    strncpy(mpmpath, mpmfilepath, MAXPATHLEN);
    mpmpath[MAXPATHLEN - 1] = 0;

    if(stat(mpmpath, &statbuf) < 0)
	return NULL;
    if(statbuf.st_size < FGROWSIZE)
	return NULL;
    mem = (char *)mpm_sbrk(mpmpath, 0);	/* Handles alignment for us */
    if(mem == NULL)
	return NULL;
/*    mpm_lock_breaker(); */
    mpm_rd_lock(3);
    mpmseq = state->seq;
    addr = state->firstblock;
    mpm_rd_unlock();
    return addr;
} /* mpm_attach */


void *
mpm_attach_clear_sem(const char *mpmfilepath)
{
    char *addr;

    addr = (char *)mpm_sbrk(mpmfilepath, 0);	/* Handles alignment for us */
    if(state)
	mpm_init_sem();
    sema_wait(&(state->mutex));
    mpmseq = ++(state->seq);
    sema_post(&(state->mutex));
    return mpm_attach(mpmfilepath);
} /* mpm_attach_clear */


static void
mpm_lock_breaker()
{

    time_t now;
    pid_t pid;
    pid_t slpid;
    return;

    now = time(NULL);

    /* Fork off a new process to handle lock breaking if a lock breaker */
    /* hasn't registered itself as active in the last 10 seconds */

    /*
    if(now < state->breaker_tick + 2)
	return;
    */

    pid = fork();
    if(pid != 0) {
	/*
	** parent
	*/
	return;
    }
    
    mpm_debug("Lock Breaker Started\n");
    
    /* Now in the child */
    pid = getpid();
    mpm_debug("Pid = %d\n", (int) pid);

    /* close stdin */
    close(STDIN_FILENO);
    
    /* Separate from controlling process */
    slpid = setpgrp();
    mpm_debug("Slpid = %d\n", (int) slpid);

    state->breaker_pid = pid;
    for(;;sleep(1)) {
	now = time(NULL);
	state->breaker_tick = now;

	/* Someone else is running too - exit */
	/* Problem with this is, we never reap in the parent, and this has
	   the fd for the shared memory open :( */
	if(state->breaker_pid != pid) {
	    mpm_debug("Another lock breaker running - exiting\n");
	    
	    exit(0);
	}

	/* Only really concerned with breaking locks for writers if say */
	/* a reader has died holding a lock, or a writer has died holding */
	/* a lock.  If a reader dies holding a lock, it won't be blocking */
	/* other readers, so we can't really pick that up except for the */
	/* affect this has by writers blocking forever. */

	/* We are going to assume that nobody will die while holding */
	/* the mutex.  If they do, we will need to abort all processes, */
	/* reset the mutex & semaphores, and restart - bummer huh! */

	if(state->brkwrlocktime > 0 && now > state->brkwrlocktime) {
	    
	    mpm_debug("Need to break a lock (brkwrlocktime expired)\n");
#ifdef DARIUS_DEBUG
	    mpm_debug("wr_wait = %d, rd_wait = %d, wr_act = %d, rd_act = %d\n",state->wr_wait,state->rd_wait,state->wr_act,state->rd_act);
#endif
	    /* Break mutex */
	    if(sema_trywait(&(state->mutex)) < 0) {
		mpm_debug("Breaking mutex lock\n");
	    }
	    sema_post(&(state->mutex));
	    
	    sema_wait(&(state->mutex));
	    if(state->wr_wait == 0) {
		/* Huh?  Shouldn't happen.  Only waiting writers will */
		/* have a wait lock break time set.  Reset break time */
		/* Acquire mutex first because we could've raced to this */
		/* possibility. */
		mpm_debug("No waiting writers, but brkwrlocktime set - resetting\n");
		state->brkwrlocktime = 0;
		sema_post(&(state->mutex));
		continue;
	    }
	    if(state->wr_act > 0) {
		/* Can happen if a writer dies holding a lock */
		/* Signal to start currently waiting writer */
		mpm_debug("Writer active for too long - starting next waiting writer\n");
		state->wr_wait--;
		sema_post(&(state->wr));
		sema_post(&(state->mutex));
		continue;
	    }
	    if(state->rd_act > 0) {
		/* A reader has died while holding a read lock */
		/* Break the read lock by decrementing active reader */
		/* count and signal writer */
		mpm_debug("Reader active for too long - breaking (rd_act Decr from %d)\n",state->rd_act);
		state->rd_act--;
		sema_post(&(state->mutex));
		continue;
	    }
	    /* How did we get here? We didn't pick up a condition */
	    /* that caused a lock hang that's how. */

	    mpm_debug("Unknown deadlock type on writers\n");
	    mpm_debug("Recommend aborting all processes\n");
	    
	    sema_post(&(state->mutex));
	}
    }
} /* mpm_lock_breaker */


static freepage *
claim_freepage()
{
int i;
freepage *fpage;

    /* Find a freepage with some free freeptrs. If none - allocate */

    if(state->usedfreepages != NULL) {
	fpage = state->usedfreepages;
    } else if(state->freefreepages != NULL) {
	fpage = state->freefreepages;
    } else {
	fpage = (freepage *)claim_pages(1);
	fpage->freefreeptrs = FREEPTRSPERFREEPAGE;
	for(i = 0; i < FREEPTRSPERFREEPAGE; i++)
	    fpage->freeptrs[i].page = NULL;
	fpage->next = state->freefreepages;
	state->freefreepages = fpage;
    }
    return fpage;
} /* claim_freepage */


static void
unlink_freepage(freepage *fpage)
{
    if(state->freefreepages == fpage) {
	if((state->freefreepages = fpage->next) != NULL)
	    state->freefreepages->prev = NULL;
	return;
    }
    if(state->usedfreepages == fpage) {
	if((state->usedfreepages = fpage->next) != NULL)
	    state->usedfreepages->prev = NULL;
	return;
    }
    if(state->fullfreepages == fpage) {
	if((state->fullfreepages = fpage->next) != NULL)
	    state->fullfreepages->prev = NULL;
	return;
    }
    if(fpage->next != NULL)
	fpage->next->prev = fpage->prev;
    if(fpage->prev != NULL)
	fpage->prev->next = fpage->next;
} /* unlink_freepage */


static void
link_freepage(freepage *fpage)
{
freepage *tmp;

    fpage->prev = NULL;
    if(fpage->freefreeptrs == 0) {
	if((fpage->next = state->fullfreepages) != NULL)
	    state->fullfreepages->prev = fpage;
	state->fullfreepages = fpage;
	return;
    }
    if(fpage->freefreeptrs < FREEPTRSPERFREEPAGE) {
	if((fpage->next = state->usedfreepages) != NULL)
	    state->usedfreepages->prev = fpage;
	state->usedfreepages = fpage;
	return;
    }
    if(state->freefreepages == NULL) {
	fpage->next = state->freefreepages;
	state->freefreepages = fpage;
	return;
    }
    if(fpage < state->freefreepages) {
	tmp = state->freefreepages;
	fpage->next = state->freefreepages->next;
	state->freefreepages = fpage;
	free_pages((char *)tmp, 1);
    } else
	free_pages((char *)fpage, 1);
} /* link_freepage */


static void
relink_freepage(freepage *fpage)
{
    /* Unlink this freepage and then link the freepage again */

    unlink_freepage(fpage);
    link_freepage(fpage);
} /* relink_freepage */


/*
static void
free_freepage(freepage *fpage)
{
    free_pages((char *)fpage, 1);
} */ /* free_freepage */


static freeptr *
claim_freeptr()
{
int i, j;
freepage *fpage;

    fpage = claim_freepage();
    for(i = 0; i < FREEPTRSPERFREEPAGE; i++)
	if(fpage->freeptrs[i].page == NULL)
	    break;
    if(i == FREEPTRSPERFREEPAGE) {	/* BAD! Couldn't find freeptr. Put */
	fpage->freefreeptrs = 0;	/* back free page and go again */
	relink_freepage(fpage);
	return claim_freeptr();
    }
    j = fpage->freefreeptrs;
    fpage->freefreeptrs--;
    if(j == FREEPTRSPERFREEPAGE || j == 1)
	relink_freepage(fpage);
    return (fpage->freeptrs + i);
} /* claim_freeptr */


static void
free_freeptr(freeptr *fptr)
{
int i;
freepage *fpage;
char *page;

    /* Unlink fptr from freepagelist */

    if(state->freepagelist == fptr)
	state->freepagelist = fptr->next;
    else
	fptr->prev->next = fptr->next;
    if(fptr->next != NULL)
	fptr->next->prev = fptr->prev;

    fptr->page = NULL;
    fptr->numpages = 0;

    /* Find page this freeptr belongs to and update it */

    page = (char *)fptr;
    page -= ((long)fptr % SYSPAGESIZE);
    fpage = (freepage *)page;
    i = fpage->freefreeptrs++;
    if(i == 0 || i == (FREEPTRSPERFREEPAGE - 1))
	relink_freepage(fpage);
} /* free_freeptr */


static void
put_freeptr(freeptr *fptr)
{
register freeptr *tmp;
char *mem;

    if(state->freepagelist == NULL || fptr->page < state->freepagelist->page) {
	if((fptr->next = state->freepagelist) != NULL)
	    state->freepagelist->prev = fptr;
	state->freepagelist = fptr;
	fptr->prev = NULL;
    } else {
	for(tmp = state->freepagelist; tmp->next != NULL; tmp = tmp->next)
	    if(fptr->page < tmp->next->page)
		break;
	fptr->prev = tmp;
	fptr->next = tmp->next;
	tmp->next = fptr;
	if(fptr->next != NULL)
	    fptr->next->prev = fptr;
    }

    /* Merge adjacent freeptrs if possible */

    if((tmp = fptr->prev) != NULL)
	if((tmp->page + (tmp->numpages * SYSPAGESIZE)) == fptr->page) {
	    fptr->page = tmp->page;
	    fptr->numpages += tmp->numpages;
	    free_freeptr(tmp);
	}
    if((tmp = fptr->next) != NULL)
	if((fptr->page + (fptr->numpages * SYSPAGESIZE)) == tmp->page) {
	    fptr->numpages += tmp->numpages;
	    free_freeptr(tmp);
	}

    /* Give memory back to the system if we can */

    if(fptr->next == NULL)
	if((fptr->page + (fptr->numpages * SYSPAGESIZE)) == ((char *)pa + state->msize)) {
	    mem = mpm_sbrk(mpmpath, -(fptr->numpages * SYSPAGESIZE));
	    free_freeptr(fptr);
	}
} /* put_freeptr */


static void
free_pages(char *page, int numpages)
{
  freeptr *fptr;

    fptr = claim_freeptr();

    fptr->page = page;
    fptr->numpages = numpages;

    put_freeptr(fptr);
} /* free_pages */


static char *
claim_pages(int numpages)
{
register freeptr *curr, *best;
register int size;
register char	*mem;

    size = numpages * SYSPAGESIZE;
    if(state->freepagelist == NULL) {
	mem = (char *)mpm_sbrk(mpmpath, size);
	return (void *)mem;
    }
    for(best = NULL, curr = state->freepagelist; curr != NULL; curr = curr->next) {
	if(curr->numpages == numpages) {
	    best = curr;
	    break;
	}
	if(curr->numpages < numpages)
	    continue;
	if(best == NULL) {
	    if(curr->numpages > numpages)
		best = curr;
	    continue;
	}
	if(best->numpages > curr->numpages)
	    best = curr;
    }
    if(best == NULL) {
	mem = (char *)mpm_sbrk(mpmpath, size);
	return (void *)mem;
    }
    if(best->numpages == numpages) {
	mem = (char *)best->page;
	free_freeptr(best);
	return (void *)mem;
    }
    /* best->numpages > numpages - Split best up */
    mem = best->page;
    best->page = mem + size;
    best->numpages = best->numpages - numpages;
    return mem;
} /* claim_pages */


static int
get_bucketnum(int numbytes)
{
int bitpos;
int tpos;
int incr;
int base;

    if(numbytes < MINALLOCSIZE)
	numbytes = MINALLOCSIZE;
    for(bitpos=0,tpos=0; numbytes > (1 << tpos); tpos++, bitpos+=SUBSPLITS);
    bitpos -= SUBSPLITS;
    tpos--;
    base = (1 << tpos);
    incr = base / SUBSPLITS;
    for(tpos = 1; tpos <= SUBSPLITS; tpos++)
	if(numbytes <= (base + incr * tpos))
	    break;
    return bitpos + tpos;
} /* get_bucketnum */


static int
get_chunksize(int bucketnum)
{
int	bitpos;
int	tpos;
int	base;

    bitpos = bucketnum / SUBSPLITS;
    tpos = bucketnum % SUBSPLITS;
    base = 1 << bitpos;
    return base + ((base / SUBSPLITS) * tpos);
} /* get_chunksize */


static void
link_malpage(malpage *mpage)
{
    mpage->next = state->used_malpages;
    state->used_malpages = mpage;
} /* link_malpage */


static void
unlink_malpage(malpage *mpage)
{
malpage *prev = NULL;

    if(state->used_malpages == mpage) {
	state->used_malpages = mpage->next;
	return;
    }
    for(prev = state->used_malpages; prev->next != NULL; prev = prev->next)
	if(prev->next == mpage) {
	    prev->next = mpage->next;
	    return;
	}
} /* unlink_malpage */


static malpage *
get_malpage()
{
malpage *mpage;
int i;

    mpage = (malpage *)claim_pages(1);
    mpage->freemalptrs = MALPTRSPERMALPAGE;
    for(i = 0; i < MALPTRSPERMALPAGE; i++)
	mpage->malptrs[i].page = NULL;
    return mpage;
} /* get_malpage */


static void
link_malptr(malptr *mptr, int bucketnum)
{
    mptr->next = state->used_malptrs[bucketnum];
    state->used_malptrs[bucketnum] = mptr;
} /* link_malptr */


static void
unlink_malptr(malptr *mptr, int bucketnum)
{
malptr *prev = NULL;

    if(state->used_malptrs[bucketnum] == mptr) {
	state->used_malptrs[bucketnum] = mptr->next;
	return;
    }
    for(prev = state->used_malptrs[bucketnum]; prev->next != NULL; prev = prev->next)
	if(prev->next == mptr) {
	    prev->next = mptr->next;
	    return;
	}
    /* If we get here the memory space has probably been corrupted somehow */
    /* We should introduce a method of telling the processes this and force */
    /* a restart */
    
    mpm_debug("Couldn't find mptr to unlink\n");
    
} /* unlink_malptr */


static char *
get_memory(int numbytes, int align)
{
register int i;
register unsigned char *a;
register malptr *mptr;
register unsigned char *b;
register malptr *emptr;
int	chunksize;
int	bucketnum;
malpage *mpage;
int numchunks;
int numpages;

    if(!state)
	return NULL;

    if(align > SYSPAGESIZE)
	return NULL;
    bucketnum = get_bucketnum(numbytes);
    chunksize = get_chunksize(bucketnum);
    for(mptr = state->used_malptrs[bucketnum]; mptr != NULL; mptr = mptr->next) {
	if(!align) {
	    a = mptr->use;
	    b = a + (mptr->numchunks / 8);
	    while(a < b && *a == 255) a++;
	    i = (a - mptr->use) * 8;
	    while(i < mptr->numchunks && BITSET(mptr->use, i)) i++;
	    if(i == mptr->numchunks) {
		mptr->freechunks = 0;
		unlink_malptr(mptr, bucketnum);
		continue;
	    }
	} else {
	    for(i = 0; i < mptr->numchunks; i++)
		if(!BITSET(mptr->use, i))
		    if(((long)(mptr->page + (i * mptr->chunksize)) % align)==0)
			break;
	    if(i == mptr->numchunks)
		continue;
	}
	mptr->freechunks--;
	if(mptr->freechunks == 0)
	    unlink_malptr(mptr, bucketnum);
	SETBIT(mptr->use, i);
	return (mptr->page + (i * mptr->chunksize));
    }

    /* Couldn't find a good bit of free memory. Grab a free mptr */

    if(state->used_malpages != NULL) {
	mpage = state->used_malpages;
	mptr = mpage->malptrs;
	emptr = mptr + MALPTRSPERMALPAGE;
	while(mptr < emptr && mptr->page != NULL)
	    mptr++;
    } else {
	mpage = get_malpage();
	link_malpage(mpage);
	mptr = mpage->malptrs;
    }

    mpage->freemalptrs--;
    if(mpage->freemalptrs == 0)
	unlink_malpage(mpage);

    /* Allocate space for the malptr and add it to the appropriate bucket */

    if(chunksize > MAXDEFAULTSPACE) {
	numpages = (chunksize + SYSPAGESIZE - 1) / SYSPAGESIZE;
	numchunks = 1;
    } else {
	numchunks = MAXDEFAULTSPACE / chunksize;
	if(numchunks > DEFAULTNUMCHUNKS)
	    numchunks = DEFAULTNUMCHUNKS;
	numpages = ((numchunks * chunksize) + (SYSPAGESIZE / 2)) / SYSPAGESIZE;
	numchunks = (numpages * SYSPAGESIZE) / chunksize;
	if(numchunks > DEFAULTNUMCHUNKS) {
	    numpages = (DEFAULTNUMCHUNKS * chunksize) / SYSPAGESIZE;
	    numchunks = (numpages * SYSPAGESIZE) / chunksize;
	}
    }
    mptr->page = claim_pages(numpages);
    mptr->numpages = numpages;
    mptr->numchunks = numchunks;
    mptr->freechunks = numchunks;
    mptr->chunksize = chunksize;
    for(i = 0; i < DEFAULTNUMCHUNKS / 8; i++)
	mptr->use[i] = 0;
    link_malptr(mptr, bucketnum);

    /* Now try to find a free bit of memory from this malptr */
    /* Starting from a fresh malptr so if not align then return first bit */

    if(!align) {
	mptr->freechunks--;
	if(mptr->freechunks == 0) {
	    unlink_malptr(mptr, bucketnum);
	}
	SETBIT(mptr->use, 0);
	splay_insert(&(state->rootmalptr), mptr);
	return mptr->page;
    }

    for(i = 0; i < mptr->numchunks; i++)
	if(!BITSET(mptr->use, i))
	    if(((long)(mptr->page + (i * mptr->chunksize)) % align)==0)
		break;
    if(i < mptr->numchunks) {
	mptr->freechunks--;
	if(mptr->freechunks == 0)
	    unlink_malptr(mptr, bucketnum);
	SETBIT(mptr->use, i);
	splay_insert(&(state->rootmalptr), mptr);
	return (mptr->page + (i * mptr->chunksize));
    }

    /* Assume user has some screwed up value here.  We have values which will */
    /* align happily to any power of 2 up to SYSPAGESIZE.  If they want */
    /* something else, just tell 'em to nick off */

    unlink_malptr(mptr, bucketnum);
    free_pages(mptr->page, numpages);
    mptr->page = NULL;
    if(mpage->freemalptrs == 0)
	link_malpage(mpage);
    mpage->freemalptrs++;
    unlink_malpage(mpage);
    free_pages((char *)mpage, 1);
    return NULL;
} /* get_memory */


static void
free_memory(char *addr)
{
malptr **pptr;
malptr *mptr;
malpage *mpage;
int bitpos;
int bucketnum;

    if(!state)
	return;

    /* Find the malptr which looks after `addr' */

    if((pptr = splay_find_splay(&(state->rootmalptr), addr)) == NULL)
	return;
    mptr = *pptr;
    if(mptr == NULL)
	return;

    /* Check addr's bit.  If unused, then just return */

    bitpos = (long)(addr - mptr->page) / mptr->chunksize;
    if(!BITSET(mptr->use, bitpos))
	return;

    /* Mark the bit in malptr to be unused.  Increment the free count */
    /* If free count hits max then call routine to mark this malptr as */
    /* unused */

    CLRBIT(mptr->use, bitpos);
    bucketnum = get_bucketnum(mptr->chunksize);
    if(mptr->freechunks == 0)
	link_malptr(mptr, bucketnum);
    mptr->freechunks++;
    if(mptr->freechunks < mptr->numchunks)
	return;

    /* Freed up entire set of chunks in malptr. Free the malptr */

    splay_remove(&(state->rootmalptr), (char *)mptr->page);
    unlink_malptr(mptr, bucketnum);
    free_pages(mptr->page, mptr->numpages);
    mptr->page = NULL;
    mpage = (malpage *)((char *)mptr - ((long)mptr % SYSPAGESIZE));
    if(mpage->freemalptrs == 0)
	link_malpage(mpage);
    mpage->freemalptrs++;
    if(mpage->freemalptrs < MALPTRSPERMALPAGE)
	return;

    /* Freed up all malptrs in the malpage.  Free the malpage */

    unlink_malpage(mpage);
    free_pages((char *)mpage, 1);
    return;
} /* free_memory */


void *
mpm_alloc(size_t size)
{
    void *addr;

    if(mpm_wr_lock(5) < 0) {
	return NULL;
    }
    addr = (void *)get_memory(size, 0);
    if(state->firstblock == NULL)
	state->firstblock = addr;
    mpm_wr_unlock();
    return addr;
} /* malloc */


void *
mpm_calloc(size_t nelem, size_t elsize)
{
    size_t size;
    char *addr;

    if(mpm_wr_lock(5) < 0) {
	return NULL;
    }
    size = nelem * elsize;
    if((addr = get_memory(size, 0)) == NULL) {
	return addr;
    }
    memset(addr, '\0', size);
    if(state->firstblock == NULL)
	state->firstblock = addr;
    mpm_wr_unlock();
    return (void *)addr;
} /* calloc */


void
mpm_free(void *addr)
{
    if(mpm_wr_lock(5) < 0)
	return;
    free_memory((char *)addr);
    mpm_wr_unlock();
} /* free */


char *
mpm_strdup(const char *str)
{
register const char *a;
register char *b;
register char *new;

    if(!str) return NULL;
    if(mpm_wr_lock(5) < 0)
	return NULL;
    a = str;
    while(*a++);
    if((new = get_memory((int)(a - str), 0)) == NULL) {
	mpm_wr_unlock();
	return NULL;
    }
    a = str;
    b = new;
    while((*b++ = *a++));
    mpm_wr_unlock();
    return new;
} /* strdup */


void *
mpm_align(size_t alignment, size_t size)
{
    void *addr;

    if(mpm_wr_lock(5) < 0)
	return NULL;
    addr = get_memory(size, alignment);
    mpm_wr_unlock();
    return addr;
} /* memalign */


void *
mpm_realloc(void *ptr, size_t size)
{
malptr *mptr = NULL;
char *new;

    if(mpm_rd_lock(3) < 0)
	return NULL;

    if(ptr != (void *)NULL)
	if((mptr = splay_find(state->rootmalptr, ptr)) == (malptr *)NULL) {
	    mpm_rd_unlock();
	    return (void *)NULL;
	}

    if(mptr != NULL && mptr->chunksize >= size) {
	mpm_rd_unlock();
	return ptr;
    }

    mpm_rd_unlock();

    if(mpm_wr_lock(5) < 0)
	return NULL;
    if((new = get_memory(size, 0)) == NULL) {
	mpm_wr_unlock();
	return (void *)NULL;
    }

    if(ptr == (void *)NULL) {
	mpm_wr_unlock();
	return new;
    }

    memcpy(new, ptr, mptr->chunksize);
    free_memory(ptr);
    mpm_wr_unlock();

    return new;
} /* realloc */


void *
mpm_valloc(size_t size)
{
    void *addr;

    if(mpm_wr_lock(5) < 0)
	return NULL;
    addr = get_memory(size, SYSPAGESIZE);
    mpm_wr_unlock();
    return addr;
} /* valloc */


size_t
mpm_allocsize(void *ptr)
{
    malptr *mptr;

    if(mpm_rd_lock(3) < 0)
	return -1;
    if((mptr = splay_find(state->rootmalptr, ptr)) == NULL) {
	mpm_rd_unlock();
	return -1;
    }
    mpm_rd_unlock();
    return mptr->chunksize;
} /* mallocsize */


static void
mpm_check_remap()
{
    if(state == NULL)
	return;
    if(mapsize < state->fsize) {
	mpm_wr_lock(5);
	mpm_grow_file_and_map(mpmpath, 0);
	mpm_wr_unlock();
    }
} /* mpm_check_remap */


int
mpm_rd_lock(int nsec)
{
    if(!state)
	return -1;

    if(wr_lock_cnt > 0 || rd_lock_cnt > 0) {
	rd_lock_cnt++;
	return 0;
    }
    rd_lock_cnt++;
    if(mpm_rd_sem_lock(nsec) < 0) {
	rd_lock_cnt--;
	return -1;
    }
    return 0;
} /* mpm_rd_lock */


static int
mpm_rd_sem_lock(int nsec)
{
    sema_wait(&(state->mutex));
    if(mpmseq != state->seq) {
	sema_post(&(state->mutex));
	return -1;
    }
    if(state->wr_act > 0 || state->wr_wait > 0) {
	state->rd_wait++;
	state->brkwrlocktime = time(NULL) + nsec;
	sema_post(&(state->mutex));

	sema_wait(&(state->rd));

	sema_wait(&(state->mutex));
	if(state->rd_wait == 0)
	    state->brkrdlocktime = 0;
	sema_post(&(state->mutex));
    } else {
	if(state->rd_wait == 0)
	    state->brkrdlocktime = 0;
#ifdef DARIUS_DEBUG
	mpm_debug("mpm_rd_sem_lock rd_act Incr (rd_wait = %d, rd_act = %d, wr_wait = %d, wr_act = %d)\n",state->rd_wait, state->rd_act, state->wr_wait, state->wr_act);
#endif
	state->rd_act++;
	sema_post(&(state->mutex));
    }
    mpm_check_remap();
    return 0;
} /* mpm_rd_sem_lock */


int
mpm_wr_lock(int nsec)
{
    if(!state)
	return -1;

    if(wr_lock_cnt > 0) {
	wr_lock_cnt++;
	return 0;
    }

    if(rd_lock_cnt > 0)		/* Need to promote read lock */
	mpm_rd_sem_unlock();	/* Break rd semaphore lock */

    wr_lock_cnt++;
    if(mpm_wr_sem_lock(nsec) < 0) {	/* Acquire wr semaphore lock */
	wr_lock_cnt--;
	return -1;
    }
    return 0;
} /* mpm_wr_lock */


static int
mpm_wr_sem_lock(int nsec)
{
    sema_wait(&(state->mutex));
    if(mpmseq != state->seq) {
	sema_post(&(state->mutex));
	return -1;
    }
    if(state->rd_act > 0 || state->wr_act > 0 || state->wr_wait > 0) {
	state->wr_wait++;
	state->brkwrlocktime = time(NULL) + nsec;
	sema_post(&(state->mutex));

	sema_wait(&(state->wr));

        sema_wait(&(state->mutex));
	if(state->wr_wait == 0)
	    state->brkwrlocktime = 0;
	sema_post(&(state->mutex));
    } else {
#ifdef DARIUS_DEBUG
	mpm_debug("mpm_wr_sem_lock wr_act Incr (rd_wait = %d, rd_act = %d, wr_wait = %d, wr_act = %d)\n",state->rd_wait, state->rd_act, state->wr_wait, state->wr_act);
#endif
	state->wr_act++;
	if(state->wr_wait == 0)
	    state->brkwrlocktime = 0;
	sema_post(&(state->mutex));
    }
    mpm_check_remap();
    return 0;
} /* mpm_wr_sem_lock */


int
mpm_rd_unlock()
{
    if(!state)
	return -1;
    if(rd_lock_cnt <= 0)
	return -1;
    if(wr_lock_cnt > 0) {
	rd_lock_cnt--;
	return 0;
    }

    rd_lock_cnt--;
    if(rd_lock_cnt > 0)
	return 0;
    mpm_rd_sem_unlock();
    return 0;
} /* mpm_rd_unlock */


static int
mpm_rd_sem_unlock()
{
    sema_wait(&(state->mutex));
    if(mpmseq != state->seq) {
	sema_post(&(state->mutex));
	return -1;
    }
    if(state->rd_act > 0) {
#ifdef DARIUS_DEBUG
	mpm_debug("mpm_rd_sem_unlock rd_act Decr (rd_wait = %d, rd_act = %d, wr_wait = %d, wr_act = %d)\n",state->rd_wait, state->rd_act, state->wr_wait, state->wr_act);
#endif
	state->rd_act--;
    }
    if(state->wr_wait > 0) {
	if(state->rd_act == 0) {
#ifdef DARIUS_DEBUG
	    mpm_debug("mpm_rd_sem_unlock wr_act Incr (rd_wait = %d, rd_act = %d, wr_wait = %d, wr_act = %d)\n",state->rd_wait, state->rd_act, state->wr_wait, state->wr_act);
#endif
	    state->wr_wait--;
	    state->wr_act++;
	    sema_post(&(state->wr));
	}
    } else {
	while(state->rd_wait > 0) {
#ifdef DARIUS_DEBUG
	    mpm_debug("mpm_rd_sem_unlock rd_act Incr (rd_wait = %d, rd_act = %d, wr_wait = %d, wr_act = %d)\n",state->rd_wait, state->rd_act, state->wr_wait, state->wr_act);
#endif
	    state->rd_wait--;
	    state->rd_act++;
	    sema_post(&(state->rd));
	}
    }
    sema_post(&(state->mutex));
    return 0;
} /* mpm_rd_sem_unlock */


int
mpm_wr_unlock()
{
    if(!state)
	return -1;
    if(wr_lock_cnt <= 0)
	return -1;
    wr_lock_cnt--;
    if(wr_lock_cnt > 0)
	return 0;

    mpm_wr_sem_unlock();

    if(rd_lock_cnt > 0)		/* Do a read lock if outstanding */
	return mpm_rd_sem_lock(30);
    return 0;
} /* mpm_wr_unlock */


static int
mpm_wr_sem_unlock()
{
    sema_wait(&(state->mutex));
    if(mpmseq != state->seq) {
	mpm_debug("Write Unlock Aborted - mpmseq != state->seq\n");
	sema_post(&(state->mutex));
	return -1;
    }
    if(state->wr_act > 0) {
#ifdef DARIUS_DEBUG
	mpm_debug("mpm_wr_sem_unlock wr_act Decr (rd_wait = %d, rd_act = %d, wr_wait = %d, wr_act = %d)\n",state->rd_wait, state->rd_act, state->wr_wait, state->wr_act);
#endif
	 state->wr_act--;
    }
    if(state->rd_wait > 0) {
	 while(state->rd_wait > 0) {
#ifdef DARIUS_DEBUG
	    mpm_debug("mpm_wr_sem_unlock rd_act Incr (rd_wait = %d, rd_act = %d, wr_wait = %d, wr_act = %d)\n",state->rd_wait, state->rd_act, state->wr_wait, state->wr_act);
#endif
	     state->rd_wait--;
	     state->rd_act++;
	     sema_post(&(state->rd));
	 }
    } else {
	if(state->wr_wait > 0) {
#ifdef DARIUS_DEBUG
	    mpm_debug("mpm_wr_sem_unlock wr_act Incr (rd_wait = %d, rd_act = %d, wr_wait = %d, wr_act = %d)\n",state->rd_wait, state->rd_act, state->wr_wait, state->wr_act);
#endif
	    state->wr_wait--;
	    state->wr_act++;
	    sema_post(&(state->wr));
	}
    }
    sema_post(&(state->mutex));
    return 0;
} /* mpm_wr_sem_unlock */


#ifdef DEBUG_MALLOC
static void
splay_check(malptr *p, char *s, char *e)
{
    if(p == NULL)
	return;
    if((s >= p->page) && s < (p->page + p->numchunks * p->chunksize)) {
	mpm_debug("1:Malptr address clash\n");
	mpm_debug("s = %d, e = %d\n", s, e);
	mpm_debug("ps = %d, pe = %d\n", p->page, p->page + p->numchunks * p->chunksize);
    }
    if((e > p->page) && e <= (p->page + p->numchunks * p->chunksize)) {
	mpm_debug("2:Malptr address clash\n");
	mpm_debug("s = %d, e = %d\n", s, e);
	mpm_debug("ps = %d, pe = %d\n", p->page, p->page + p->numchunks * p->chunksize);
    }
    if(s < p->page && e >= p->page + p->numchunks * p->chunksize)
	mpm_debug("3:Malptr address clash\n");
    splay_check(p->left, s, e);
    splay_check(p->right, s, e);
} /* splay_check */
#endif


static void
splay_insert(malptr **root, malptr *mptr)
{
register malptr **p;

	mptr->left = NULL;
	mptr->right = NULL;
#ifdef DEBUG_MALLOC
	splay_check(*root, mptr->page, mptr->page + mptr->numchunks * mptr->chunksize);
#endif
	p = splay_find_splay(root, mptr->page);
	*p = mptr;
} /* splay_insert */


static malptr *
splay_find(malptr *mptr, char *addr)
{
    /* Scan down tree until we find the node */

    while(mptr != NULL) {
	if((addr >= mptr->page) && (addr < (mptr->page + mptr->chunksize * mptr->numchunks)))
	    break;
	if(addr < mptr->page)
	    mptr = mptr->left;
	else
	    mptr = mptr->right;
    }
    return mptr;
} /* splay_find */


static malptr **
splay_find_splay(malptr **p, char *addr)
{
register malptr *mptr;

    for(;;) {
	if(*p == NULL)
	    break;
	mptr = *p;
	if((addr >= mptr->page) && (addr < (mptr->page + mptr->chunksize * mptr->numchunks)))
	     break;
	if(addr < mptr->page) {		/* Left subtree case */
	    if(mptr->left == NULL)
		return &(mptr->left);
	    *p = mptr->left;
	    mptr->left = (*p)->right;
	    (*p)->right = mptr;
	} else {
	    if(mptr->right == NULL)
		return &(mptr->right);
	    *p = mptr->right;
	    mptr->right = (*p)->left;
	    (*p)->left = mptr;
	}
	if((addr >= (*p)->page) && (addr < ((*p)->page + (*p)->chunksize * (*p)->numchunks)))
	    break;
	if(addr < (*p)->page)
	    p = &((*p)->left);
	else
	    p = &((*p)->right);
    }
    return p;
} /* splay_find_splay */


static malptr *
splay_remove(malptr **p, char *addr)
{
register malptr *mptr;
register malptr *prev, *curr;

    p = splay_find_splay(p, addr);
    if((mptr = *p) == NULL)
	return mptr;

    /* Check left edge case */

    if(mptr->left == NULL) {
	*p = mptr->right;
	return mptr;
    }

    prev = mptr->left;
    curr = prev->right;

    /* Check parent of a right edge case on left subtree */

    if(curr == NULL) {
	prev->right = mptr->right;
	*p = prev;
	return mptr;
    }

    /* Now find the right most child of the left subtree */

    for(; curr->right != NULL; prev = curr, curr = curr->right);

    prev->right = curr->left;
    curr->left = mptr->left;
    curr->right = mptr->right;
    *p = curr;
    return mptr;
} /* splay_remove */


void mpm_debug_init(const char* client) {
    
    if(client == NULL) {
	client = "NULL";
    }
    client_name = strdup(client);
    
    debug_f = fopen("/var/log/mpmalloc.log", "a+");
    if (debug_f == NULL) {
	debug_f = fopen("/var/tmp/mpmalloc.log", "a+");
	if(debug_f == NULL) {
	    debug_f = stdout;
	}
    }
    return;
}

void mpm_debug_close() {
    if(debug_f && debug_f != stdout) fclose(debug_f);
}
    
static void mpm_debug(const char *format, ...) {
    char*	timestr;
    time_t	t;
    va_list	ap;
    
    /* va_start(ap,); */
    va_start(ap, *format);
    
    if(debug_f == NULL) {
	mpm_debug_init(NULL);
    }
    
    if(client_name == NULL) {
	client_name = "NULL";
    }
    
    t = time((time_t*) NULL);
    timestr = ctime(&t);
    if(timestr == NULL) {
	return ;
    }
    
    timestr[24] = '\0';
    fprintf(debug_f, "%s - MPM_ALLOC[%s-%d] - ", timestr, client_name, (int) getpid());
    
    vfprintf(debug_f, format, ap);
    fflush(debug_f);
    va_end(ap);
    
    return;
    
}
#endif

