#include <pthread.h>
#include <sys/errno.h>

#define MAX_JOBS 32

/* pthread error messages */
#define PML_ERR "pthread_mutex_lock error\n"
#define PMU_ERR "pthread_mutex_unlock error\n"
#define PCW_ERR "pthread_cond_wait error\n"
#define PCB_ERR "pthread_cond_broadcast error\n"

extern int errno;

/* pthread macros */
#ifdef THREAD_DEBUG
#define PT_MUTEX_LOCK(x) fprintf(stderr,"[LOCK ON] %s:%d\n", __FILE__, __LINE__); if (pthread_mutex_lock(x) != 0) fprintf(stderr, PML_ERR);
#define PT_MUTEX_UNLOCK(x) fprintf(stderr,"[LOCK OFF] %s:%d\n", __FILE__, __LINE__); if (pthread_mutex_unlock(x) != 0) fprintf(stderr,PMU_ERR);
#define PT_COND_WAIT(x,y) fprintf(stderr,"[WAIT ON] %s:%d\n", __FILE__, __LINE__); if (pthread_cond_wait(x, y) != 0) fprintf(stderr,PCW_ERR);
#define PT_COND_BROAD(x) fprintf(stderr,"[BROADCAST] %s:%d\n", __FILE__, __LINE__); if (pthread_cond_broadcast(x) != 0) fprintf(stderr,PCB_ERR);
#else
#define PT_MUTEX_LOCK(x) if (pthread_mutex_lock(x) != 0) fprintf(stderr, PML_ERR);
#define PT_MUTEX_UNLOCK(x) if (pthread_mutex_unlock(x) != 0) fprintf(stderr,PMU_ERR);
#define PT_COND_WAIT(x,y) if (pthread_cond_wait(x, y) != 0) fprintf(stderr,PCW_ERR);
#define PT_COND_BROAD(x) if (pthread_cond_broadcast(x) != 0) fprintf(stderr,PCB_ERR);
#endif

/* Target state */
enum targetState {NEW, LIVE, STALE};
typedef enum {JOB, WORKER} threadtype;

/* Typedefs */
typedef struct worker_struct {
    int index;
    int target_ix;
    joblist_t *job;
    // threadtype ttype; 
    pthread_t thread;
    struct crew_struct *crew;
} worker_t;
/*
typedef struct tjob_struct {
    int index;
    struct crew_struct *crew;
} tjob_t;
*/
typedef struct crew_struct {
    int max_workers;
    int job_count;
    int work_count;
    int threadindex;
    config_t *config;
    worker_t member[MAX_THREADS];
    struct taskresult_struct taskresults[MAX_THREADS];
    // tjob_t jobs[1024];
    pthread_mutex_t mutex;
    pthread_cond_t done;
    pthread_cond_t go;
    pthread_cond_t worker_done;		// A worker has completed
    pthread_cond_t workers_go;		// All workers start
    pthread_cond_t ready;
} crew_t;

int scheduler(config_t *config , joblist_t *joblist);
void *workerjob(void *thread_args);
void *sig_handler(void *arg);
