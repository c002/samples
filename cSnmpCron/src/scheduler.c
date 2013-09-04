/***
 * scheduler.c
 *	This schedules jobs and workers at miniute resolution like cron.
 *
 *	- If a job runs too long and hits its next runtime, it is skipped.
 *	- New jobs will always start and its workers will run providing
 *	  enough threads are available.
 *
 * 
 * 
 *
 ***/

#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <dlfcn.h>
#include <link.h>
#include <common.h>
#include <signal.h>
#include <job.h>
#include <scheduler.h>

#define MODULE_NAME "scheduler.c"

#define TDEBUG 0	/* Test threading. run threads as fast as possible to help catch 
			   bugs and mem leaks */
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT   ((void *) 0)
#endif

#ifdef TRACE
#define trace() va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"trace [%s]:%d", __FILE__, __LINE__)
#else
#define trace() 
#endif

#define RETURN(t) va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG,"Error return [%s]:%d", __FILE__, __LINE__); return(t)

static int sigged=0;
static int waiting;
static int reload;
static int quit;
static int fault;

static pthread_mutex_t snmpmutex_main;         // Locking inside SNMP code on socket RCV

extern time_t lastloadtime;		// time config was last loaded;
extern char targetfilepath[];

#define MAXSLOTS 1024
static int threadslots[MAXSLOTS];

static char version[] ="

bool check_runtime(int runtime, const char *runstr, char **last);
void *threadjob(void *thread_args);

static void sig_coredump(int sig);

int getslot(int threadslots[]);
void freeslot(int threadslots[], int slot);

static crew_t crew;
static char *datadir;

/***
 * main process to handles the scheduling of jobs and threads.
 *  
 */
int scheduler(config_t *config , joblist_t *joblist)
{
    time_t target_time, sec_wait, t;
    int target_hour, target_min, threadcount, respawn=0;
    joblist_t *job;
    int result, BUFSIZE=64, rm, rh;
    sigset_t signal_set;
    pthread_t sig_thread;
    char buf[BUFSIZE+1];
    struct tm *pt, pt_r;
    pthread_attr_t thread_attr;
    struct timespec rqtp;
    size_t szstack;
    int res=0;	
    struct stat statbuf;
    char *last;

    // for coredumps also
    datadir = config->datadir;

    //For locking things in the SNMP code.
    pthread_mutex_init(&(snmpmutex_main), NULL);

    pthread_mutex_init(&(crew.mutex), NULL);
    pthread_cond_init(&(crew.done), NULL);
    pthread_cond_init(&(crew.worker_done), NULL);
    pthread_cond_init(&(crew.go), NULL);
    pthread_cond_init(&(crew.workers_go), NULL);
    pthread_cond_init(&(crew.ready), NULL);
    threadcount=0;

    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGHUP);
    sigaddset(&signal_set, SIGALRM);
    sigaddset(&signal_set, SIGUSR1);
    sigaddset(&signal_set, SIGUSR2);
    sigaddset(&signal_set, SIGTERM);
    sigaddset(&signal_set, SIGINT);
    sigaddset(&signal_set, SIGQUIT);
//XXX
    sigaddset(&signal_set, SIGSEGV);
    sigaddset(&signal_set, SIGBUS);
//XXX end

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&sig_thread, &thread_attr, sig_handler, (void *) &(signal_set)) != 0)
        printf("pthread_create error\n");

    crew.work_count = 0;
    crew.max_workers=config->maxworkers;
    crew.job_count=0;
    crew.config = config;
    waiting=0;
    int i=0;

    rqtp.tv_sec=0;
    rqtp.tv_nsec=0;

    while (1) {			
       	// PT_MUTEX_LOCK(&(crew.mutex));
	t=time(NULL);
	target_time = (time_t) ((time(NULL) /60) + 1) * 60; 

	pt=localtime_r(&target_time, &pt_r);
	strftime(&buf[0], BUFSIZE, "%k",pt);
	
	target_hour= atoi(buf);
	/* for testing, set mins to seconds. */
	if (TDEBUG) {
	   pt=localtime_r(&t, &pt_r);
	   strftime(&buf[0], BUFSIZE, "%S",pt);
	} else {
	    pt=localtime_r(&target_time, &pt_r);
            va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "tmin=%d", pt->tm_min); 
	    strftime(&buf[0], BUFSIZE, "%M",pt);
        }
	target_min = atoi(buf);
       	// PT_MUTEX_UNLOCK(&(crew.mutex));

	rqtp.tv_sec = target_time - time(NULL);

	exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "-------------------------------------------------------------");

        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "target_hr=%d,target_min=%d (targ=%ld),(time=%ld),(sleep=%d)", target_hour, target_min, target_time, time(NULL), rqtp.tv_sec); 
	exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "-------------------------------------------------------------");

	if (TDEBUG) {
    	    rqtp.tv_sec=0;
            rqtp.tv_nsec=400 * 1000; 		// 400us
	    res=nanosleep(&rqtp, NULL);		/* nanosleep() is threadsafe. */
	} else { 
	    res=nanosleep(&rqtp, NULL);		// sleep  here until next runtime

	    /* localtime_r used to be localtime (above).  This caused a bug with the occasional 
  	       corruption of target_min.  This appears to be Ok now, so this code
 	       is not really needed anymore. -- may08 harryr
	   */
	    t=time(NULL);
	    pt=localtime_r(&t, &pt_r);
	    strftime(&buf[0], BUFSIZE, "%M",pt);
	    if (target_min < atoi(buf))  {
	        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, "Detected target_min (%ld) < now_time (%ld), skipping (thread bug!)", target_time ,t); 
	        continue;
	    }
	}

        if (res!=0) {		// sigged
	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "nanosleep signal interupted, work is (%d)", crew.work_count); 
	}	
	/***
 	 * We're awake, Start next cycle.
	 **/

	/* Check to see if config modtime has changed */
	if (targetfilepath[0]) {
		if (stat(targetfilepath, &statbuf)==0) {
	    	     // va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "lastload=%lu, mtime=%lu", lastloadtime, statbuf.st_mtime); 
		    if (lastloadtime>0 && statbuf.st_mtime>=lastloadtime) {
	    		exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Target file modtime changed. Reloading."); 
			reload++;
		    }
		 }
	    
	}

	/*  if respawn option. Check if its @3:43. In case mem leaks are an issue. */
	if ((config->respawn==1) && (check_runtime(target_hour, "3" ,&last)) && (check_runtime(target_min,  "43", &last))) {
	      exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Respawn Time reached."); 
	      quit++;
	      respawn++;
	}

	/* Check pending signals */

	while (quit) {
	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Quit Signal received, Exiting when current work is done (%d)", crew.work_count); 
	    while (crew.work_count>0) {
		PT_COND_WAIT(&(crew.worker_done), &(crew.mutex));
        	PT_MUTEX_UNLOCK(&(crew.mutex));
	    }
	    exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Exiting scheduler."); 
	    quit--;

	    if (respawn)
	    	return(2);
	    else
	        return(0);
	}

	while (reload) {
	
	    exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "HUP Signal received, reloading config when all jobs done"); 

            PT_MUTEX_LOCK(&(crew.mutex));
	    /* Wait till all work is done before reloading */
	    while (crew.work_count>0) {
		PT_COND_WAIT(&(crew.worker_done), &(crew.mutex));
        	PT_MUTEX_UNLOCK(&(crew.mutex));
	    }

	    joblist = init_config(NULL, config, NULL);
	    crew.max_workers=config->maxworkers;
            crew.config = config;

	    exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Reloaded config."); 

	    reload--;
            PT_MUTEX_UNLOCK(&(crew.mutex));
	}

	// Check if we want to roll the logfile.
	if (config->roll && target_hour==0 && target_min==0)
	    rolllog();


        if (pthread_sigmask(SIG_BLOCK, &signal_set, NULL) != 0)
            printf("pthread_sigmask error\n");

	/* For all the jobs we know about, check if they need to be run now 
 	   Don't want to include the job threads in the work count 
	   or the actual work doesnt spread evenly. 
	*/   	 

	for (job=joblist; job!=NULL; job=job->next)
	{
	   
	    if (( check_runtime(target_hour, job->runhr, &last) &&
		 check_runtime(target_min,  job->runmin, &last))) {

		 /* There is work to do */

		 va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "check_runtime matches: hr:%d==%s, min:%d==%s", target_hour, job->runhr, target_min, job->runmin);

                PT_MUTEX_LOCK(&(crew.mutex));

		if (job->activecount>0) {
		   va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "job [%d] %s still active, not running it (crew.job_count=%d)", job->id, job->jobname, crew.job_count);
                   PT_MUTEX_UNLOCK(&(crew.mutex));
		    continue;
		}

		/* create New job thread for every job , regardless of the max number
		   of threads. This means that each worker for a job competes for
		   available threads and is not dependant on the number or order of jobs.  
		   These job threads are 'cheap' because they do little
		   anyway.
		*/
	
		i = getslot(threadslots);	/* We assign an available work slot for a thread */
		if (i<0) {
		   va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "No remaining thread slots (%d), skipping job [%d] %s", MAXSLOTS, job->id, job->jobname);

                   PT_MUTEX_UNLOCK(&(crew.mutex));
		   continue; 
		}

		job->timestamp=target_time;

        	crew.member[i].index = i;
        	crew.member[i].crew = &crew;
        	// crew.jobs[i].job = job;
        	crew.member[i].job = job;
        	//crew.member[i].ttype = JOB;
        	crew.taskresults[i].status = 0;
        	crew.taskresults[i].index = i;
        	crew.taskresults[i].snmpmutex = &(snmpmutex_main);
	
		pthread_attr_setstacksize(&thread_attr, 2000000);
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        	result = pthread_create(&(crew.member[i].thread), &thread_attr, threadjob, (void *) &(crew.member[i]));
		//pthread_attr_getstacksize(&thread_attr, &szstack);
		//va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Job Stack2 sz=%ld", szstack);
		
		switch (result) {
		    case (0) :  
				/* All good, update our tracking/stats counters */

				crew.job_count++;
				waiting++;
				
				va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, 
					"Thread=[Main], Created thread for job [%d] %s", job->id, job->jobname);
				break;
		    case (ENOMEM) : va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
					"Error Creating thread for job [%d] %s: (%d) No memory", job->id, job->jobname, result);
				    break;
		    case (EINVAL) : va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
					"Error Creating thread for job [%d] %s: (%d) Invalid value for attr", job->id, job->jobname, result);
				    break;
		    case (EPERM) : va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
					"Error Creating thread for job [%d] %s: (%d) No Permission", job->id, job->jobname, result);
				   break;
		    default: va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
				"Error Creating thread for job [%d] %s: (%d) Unknown Error", job->id, job->jobname, result);
				break;
		}
	    }
         
	    PT_MUTEX_UNLOCK(&(crew.mutex));
	}
	
	alarm(1);		// There is a case of deadlock with the while() 
        while (waiting > 0);	/* until all job threads are ready to run */ 
	alarm(0);

        if (crew.job_count>0) 
	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Ready to start job threads, job count=%d", crew.job_count);

	/* If any threads exist, 
	   Wakeup and run them so these job threads can create their worker threads. 
	   When thread completes, it destroys itself.
         */

        PT_COND_BROAD(&(crew.go));	

        PT_MUTEX_LOCK(&(crew.mutex));
        if (crew.job_count==0)
	    exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Nothing to do.");
        PT_MUTEX_UNLOCK(&(crew.mutex));

    } /* while (1) */

    /* not reached , unless testing by loopcount */
    rqtp.tv_sec=1;
    rqtp.tv_nsec=0;
    while (crew.job_count) nanosleep(&rqtp, NULL);

}

/***
 * check_runtime()
 *	takes the a comma/space seperated string
 * 	containing crontab like definitions
 * 	of times to run. Matches this against the runtime 
 *	(the current time) and returns true if it matches.
 ***/
bool check_runtime(int runtime, const char *runstr, char **last)
{
    char *token, *cprunstr;
    int datime;

    if (!runstr) return(FALSE);

    cprunstr=strdup(runstr);
    if (!cprunstr) return(FALSE);

    token=strtok_r(cprunstr,", ",last);
    while (token) {
	errno=0;

	if (strcmp(token,"*")==0)
	    break;

	datime=atoi(token);
	
	if (!datime && errno) {
	    token=NULL;
	    break;
	} else if (runtime == datime) {
	    break;
	}
        token=strtok_r(NULL,", ",last);
    }

    free(cprunstr);

    if (token)
	return(TRUE);

    return(FALSE);
}

/*
 * A job thread.  This will spawn more threads for its targets
 */
void *threadjob(void *thread_args)
{

	int i, j=0, result, index;
	worker_t *worker = (worker_t *) thread_args;
	joblist_t *job;
	crew_t *crew;
	// static crew_t *crew;
	char *target;
	pthread_attr_t thread_attr;
	struct timespec rqtp;
	size_t szstack;

	waiting--;	// thread initilises, ready to go.

        rqtp.tv_sec=1;
        rqtp.tv_nsec=0;

	crew = worker->crew;

	job = worker->job;
	target=job->targetlist[0];
 
        pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&thread_attr, 2000000);
	// va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Job Stack1 sz=%ld", szstack);

	/* Wait for a 'go' broadcast message */

        PT_MUTEX_LOCK(&(crew->mutex));
        PT_COND_WAIT(&crew->go, &crew->mutex);

	PT_MUTEX_UNLOCK(&crew->mutex);	// explicitly clear mutex

	va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Thread=[%d], Job Id=[%d] [%s] Start",worker->index, job->id, job->jobname);

	while (target!=NULL) 
	{
            PT_MUTEX_LOCK(&(crew->mutex));
	    job->activecount++;

	    if (crew->work_count >= crew->max_workers) {	// block until more worker threads are avail
	        //PT_MUTEX_UNLOCK(&crew->mutex);
                PT_COND_WAIT(&(crew->worker_done), &(crew->mutex));
	    }

            //PT_MUTEX_LOCK(&(crew->mutex));

	    i = getslot(threadslots);

            crew->member[i].target_ix = j;
            crew->member[i].index = i;
            crew->member[i].crew = crew;

            crew->member[i].job = job;
            //crew->member[i].ttype = JOB;
            crew->taskresults[i].status = 0;
            crew->taskresults[i].index = i;
            crew->taskresults[i].snmpmutex = &(snmpmutex_main);
	    crew->taskresults[i].message[0]=NULL;

	    PT_MUTEX_UNLOCK(&crew->mutex);

	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, 
		"Thread=[%d], Create Work thread for job Id=[%d] [%s], target=%s, Total Work count=[%d]", 
			worker->index,
			job->id, 
			job->jobname, 
			target, 
			crew->work_count+1);

	    // pthread_attr_getstacksize(&thread_attr, &szstack);
	    //va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Thread Stack sz=%ld", szstack);
	    trace();
       	    result = pthread_create(&(crew->member[i].thread), &thread_attr, workerjob, &(crew->member[i]));
	    if (result!=0)
	        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR,"pthread_create result %d", result);
	    trace();
	    switch (result) {
		    case (0) :  
            			PT_MUTEX_LOCK(&(crew->mutex));
	    			crew->work_count++;
	    			PT_MUTEX_UNLOCK(&crew->mutex);
				break;
		    case (ENOMEM) : va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
					"Error Creating thread for job [%d] %s: (%d) No memory", job->id, job->jobname, result);
				    exit(-1);
		    case (EINVAL) : va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
					"Error Creating thread for job [%d] %s: (%d) Invalid value for attr", job->id, job->jobname, result);
				    exit(-1);
		    case (EPERM) : va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
					"Error Creating thread for job [%d] %s: (%d) No Permission", job->id, job->jobname, result);
				   exit(-1);
		    default: va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
				"Error Creating thread for job [%d] %s: (%d) Unknown Error", job->id, job->jobname, result);
				exit(-1);
	    }
	     
            PT_MUTEX_LOCK(&(crew->mutex));
	    j++;
	    target=job->targetlist[j];
            PT_MUTEX_UNLOCK(&(crew->mutex));
	}
	trace();
	nanosleep(&rqtp, NULL);	/* wait for threads to get ready */
	
        PT_MUTEX_LOCK(&(crew->mutex));
	while (job->activecount>0) {
        	PT_COND_BROAD(&(crew->workers_go));	// Do any remaiing work
                PT_COND_WAIT(&(crew->worker_done), &(crew->mutex));
	}
	trace();
	crew->job_count--;		// This job id done.
	va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO,"Thread=[%d], job Id=[%d] [%s] Done (active=%d, jobcount=%d)", 
		worker->index,job->id, job->jobname, job->activecount, crew->job_count);

	if ((result=pthread_attr_destroy(&thread_attr))!=0)
		va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
			"Error Destroying thread for job [%d] %s (result=%d)", job->id, job->jobname, result);
	trace();	
	freeslot(threadslots,worker->index);	// This job done, free thread.

	PT_MUTEX_UNLOCK(&crew->mutex);

	PT_COND_BROAD(&(crew->go));
        PT_COND_BROAD(&(crew->done));
	trace();
}

/***
 * execute the "run" function in external modules loaded at startup
 * pass the target and data structures.
 * The SNMP stuff is all in these modules.
 * Ofcourse these modules don't necessaily have to do SNMP at all.
 *
 */ 
void *workerjob(void *thread_args)
{

	int j, i, result=0;
	char *target, funcname[1024];
        int  (*fptr)(char *, config_t *, joblist_t *, taskresult_t *);
	joblist_t *job;
	worker_t *worker = (worker_t *) thread_args;
	crew_t *crew;

        trace();
	crew = worker->crew;

        //assert(worker->index<=MAX_THREADS);
	if (worker->index>MAX_THREADS)
	     exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, "threads > MAX_THREADS.");

        trace();

	job = worker->job;

	target = job->targetlist[worker->target_ix];
	
        PT_MUTEX_LOCK(&(crew->mutex));

	va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, 
		"Thread=[%d],   Worker Start, Job id=[%d] [%s], Target %s, Total Work count=%d", 
			worker->index, 
			job->id, 
			job->jobname, 
			target,
			crew->work_count);
	PT_MUTEX_UNLOCK(&crew->mutex);

	trace();

	sprintf(funcname, "%s_run", job->modname);
	fptr = (int (*)())dlsym( job->modhandle, funcname);

	/* See if module has a run() function */
        if ((fptr = (int (*)())dlsym(RTLD_DEFAULT, funcname)) != NULL) {
              result = (*fptr)( target, crew->config, job, &(crew->taskresults[worker->index]) );
	      trace();
        } else  {
	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_FATAL, 
		"Thread=[%d],   Worker ERROR, Failed to run function %s in module [%s] Job id=[%d] [%s], Target %s",
			worker->index, 
			funcname,
			job->modpath,
			job->id, 
			job->jobname, 
			target);
            PT_MUTEX_LOCK(&(crew->mutex));
	    crew->work_count--;
	    job->activecount--;
            
	    freeslot(threadslots, worker->index);
            PT_MUTEX_UNLOCK(&(crew->mutex));
	    trace();
	    RETURN(NULL);
	} 

  	char *msg = crew->taskresults[worker->index].message;

	trace();

	if (result!=0)
	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, 
		"Thread=[%d],*  Worker for Job id=[%d] [%s] Target %s returned Error: %s",
			worker->index, 
			job->id, 
			job->jobname, 
			target,
			msg);

	va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, 
		"Thread=[%d],   Worker End, Job=[%d] [%s], Target %s, Total Work count=%d",
			worker->index, 
			job->id, 
			job->jobname, 
			target,
			(crew->work_count>0 ? crew->work_count : 0 ));

        PT_MUTEX_LOCK(&(crew->mutex));
	crew->work_count--;
	job->activecount--;
	freeslot(threadslots, worker->index);
       	PT_COND_BROAD(&(crew->worker_done));
	PT_MUTEX_UNLOCK(&crew->mutex);
	trace();
}

/* Signal Handler.  USR1 increases verbosity, USR2 decreases verbosity. 
   HUP re-reads target list */
void *sig_handler(void *arg)
{
    sigset_t *signal_set = (sigset_t *) arg;
    int sig_number;

    while (1) {
        sigwait(signal_set, &sig_number);
        switch (sig_number) {
            case SIGHUP:
	        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "HUP received, waiting=%d", waiting); 
	        //exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "HUP received"); 
                reload++;
                break;
            case SIGALRM:
		waiting=0;
		exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, "ALARM triggered");
		break;
            case SIGUSR1:
		// loglevel up
	        exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "USR1 received"); 
                break;
            case SIGUSR2:
		// loglevel down
	        exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "USR2 received"); 
                break;
            case SIGTERM:
	        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "TERM received wc=%d", crew.work_count); 
		quit++;
		if (crew.work_count==0) {
	        	exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "No current work, quiting now"); 
			exit(0);
		}
		break;
            case SIGINT:
	        exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "INT received"); 
		quit++;
		break;
            case SIGQUIT:
	        exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "QUIT received"); 
		quit++;
                break;
            case SIGSEGV:
	        exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "SEGV received"); 
                //reload++;
		sig_coredump(SIGSEGV);
		break;
            case SIGBUS:
	        exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "BUS received"); 
                //reload++;
		sig_coredump(SIGBUS);
		break;
        }
   }
}
/***
 * Use an array to store the running threads.
 * Returns the next available slot
 *
 */
int getslot(int threadslots[])
{
    int i;
    for (i=0; i<MAXSLOTS; i++)
	if (threadslots[i]==0) {
	    threadslots[i]=1;
	    return(i);
	}
    exception(MODULE_NAME, EXCEPTION_PRIORITY_ERROR, "No thread slots available.");
    RETURN(-1);	// no slots available
}

/**
 * When thread completes, clear the slot it was using.
 */
void freeslot(int threadslots[], int slot)
{
    threadslots[slot]=0;
}

/* handle all varieties of core dumping signals */
static void sig_coredump(int sig)
{
    chdir(datadir);
    signal(sig, SIG_DFL);

    sleep(10);
    fprintf(stderr, "SIGSEGV/SIGBUS recieved\n");

    kill(getpid(), sig);

    /* At this point we've got sig blocked, because we're still inside
     * the signal handler.  When we leave the signal handler it will
     * be unblocked, and we'll take the signal... and coredump or whatever
     * is appropriate for this particular Unix.  In addition the parent
     * will see the real signal we received -- whereas if we called
     * abort() here, the parent would only see SIGABRT.
     */
}

