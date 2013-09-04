#include <common.h>
#include <job.h>
#include <iniparser.h>

#define MODULE_NAME "init_config.c"

static char version[] ="

void dump_maincfg(config_t *config);
void freejoblist(joblist_t *job);

extern time_t lastloadtime;	// timestamp config was last loaded

joblist_t *init_config(char *fname, config_t *config, joblist_t *badjob)
{

    static dictionary *dict=NULL;
    config_t *conf;
    joblist_t *job;
    static joblist_t *joblist=NULL;
    static char sfname[1024]={'\0'};

    if (dict)
	iniparser_freedict(dict);

    if (fname) {
 	strcpy(sfname, fname);
    } else if (sfname[0]) 
	fname=sfname;
    dict = iniparser_load(fname);

    if (!dict) {
        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_FATAL, "Error loading config file %s", fname);
	return(NULL);
    }

    conf = load_config(dict, config);
    if (!conf) {
        exception(MODULE_NAME, EXCEPTION_PRIORITY_FATAL, "Error building config parameters");
	return(NULL);
    }

    dump_maincfg(config);

    if ( conf->maxworkers > MAX_THREADS) {
        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Too many workers, system limit is %d", MAX_THREADS);
        exit(0);
    }

    exception_init(config->loglevel, config->logfile);

    if (joblist)
	    freejoblist(joblist);

    joblist = BuildJoblist(config, dict, badjob);
    if (! joblist) {
        exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "joblist is empty.");
    }

    exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Config loaded");

    lastloadtime = (time_t) time(NULL);

    return (joblist);

}

void freejoblist(joblist_t *job)
{

    if (job) {
	if (job->next)
           freejoblist(job->next);
	//fprintf(stderr,"free job [%d] %s\n", job->id, job->jobname);
	freejob(job);
    }
}

void dump_maincfg(config_t *config)
{
    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO,
	"loglevel=%d, maxworkers=%d, maxload=%d, modulebase=%s, datadir=%s, logfile=%s (roll=%d)\n", 
	config->loglevel,
	config->maxworkers,
	config->maxload,
	(config->modulebase ? config->modulebase : "null"),
	(config->datadir ? config->datadir : "null"),
	(config->logfile ? config->logfile : "null"),
	  config->roll) ;

}
