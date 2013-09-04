#ifndef JOB_H 
#define JOB_H

#include <pthread.h>

#define MAX_TARGETS	1024

typedef enum { IDLE, ACTIVE } jobstate;

typedef struct taskresult_struct {
    int status;
    int index;
    pthread_mutex_t *snmpmutex;
    char message[1024];
} taskresult_t;

typedef struct joblist_struct { 
    int id;
    int activecount;
    time_t timestamp;	// Targettime for job
    char *jobname;	//
    int  valid;
    char *prefix;	// Prefix for outut files
    char *modpath;	// Path of loadable module
    char *modname;	// name of loadable module
    void *modhandle;	// dynamic loading handle
    char *runmin;	// Minutes to run at
    char *runhr;	// Hours to run at
    char *targetlist[MAX_TARGETS];	// Target hosts
    struct joblist_struct *next;
} joblist_t;

//#define RUNTIME_REGEX "^\([0-9,*]\+\)[[:space:]]\+\([0-9*]\+\)$"

#define CONF_RUNPAT 	"^([0-9,*]+)[[:space:]]+([0-9,*]+)$"

#define CONF_PREFIX	"prefix"
#define CONF_RUNTIME	"runtime"
#define CONF_MODNAME	"ModuleName"
#define CONF_TARGETS	"targets"
#define CONF_MODULE	"LoadModule"

#define TARGET_FROM_FILE  "file:"

joblist_t *BuildJoblist(config_t *config, dictionary *dict, joblist_t *job);
joblist_t *remove_invalid_jobs(joblist_t *joblist);
void dump_joblist(joblist_t *joblist);
joblist_t *init_config(char *fname, config_t *config, joblist_t *job);
char **load_targets_from_file(char *fname, char *targetlist[]);
void free_targetlist(char *targetlist[]);

#endif
