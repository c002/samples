/*
 *
 */
#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <stdlib.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <module.h>

void printer(config_t *config, joblist_t *job, char *target, char *fred);

int module2_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult)
{

    FILE *f;
    char *localname="testmodule";
    unsigned int seed, i;
    static unsigned int n=0;
    char msg[1024];

    if (!target) return(0);
    if (!config) return(0);
    if (!job) return(0);

    seed = time(NULL) % 65535;

    srand(seed + n);

    n= rand() % 10;

    if (! config->logfile) fprintf(stderr, "logfile is null\n");
    if (! config->datadir) fprintf(stderr, "datadir is null\n");

    fprintf(stderr, " -- [module2_run(), sleep(%d), target=%s, datadir=%s, logfile=%s] --\n", 
		         n,target,
			(config->datadir ? config->datadir : "null"), 
			(config->logfile ? config->logfile : "null"));


    taskresult->status=0;
    sprintf(msg,"success. target=%s, sleep(%d)", target, n);
    strncpy(taskresult->message,strdup(msg), sizeof(taskresult->message)-1);

    printer(config, job, target, NULL);
    fprintf(stderr, " ----\n");
    sleep(n);

    return(-1); 
}

void printer(config_t *config, joblist_t *job, char *target, char *fred)
{

    FILE *f;
    char fname[1024];
    char fpath[1024];

    if (config->datadir && job->prefix) {
	
        sprintf(fpath,"%s/%d", config->datadir, job->timestamp);
        sprintf(fname,"%s.%s.log",job->prefix, target);

        fprintf(stderr, " -- [outfile=%s/%s] --\n", fpath, fname);
    } else
	fprintf(stderr, " -- error --\n");
}
