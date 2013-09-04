/***
 *
 * cSnmpCron
 *	A multithreaded engine to run (mainly) snmp queries to
 *	targets.  It uses a simple config file to specify
 *   	runtimes , modules and targets.
 *
 * 
 * 
 *
 ***/

#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>
#include <common.h>
#include <job.h>

#define MODULE_NAME	"cSnmpCron.c"

static char version[] ="
static char source[] = "

static int loglevel=EXCEPTION_PRIORITY_DEBUG;
static char *logfile="/dev/null";

time_t lastloadtime;
char targetfilepath[PATH_MAX];

void usage(char *name);

main(int argc, char *argv[])
{
    joblist_t *joblist;
    config_t config, *conf;
    char *x, *configfile=NULL;
    sigset_t signal_set;
    char *cfg="../etc/new.conf";
    bool daemon=TRUE, roll=FALSE, respawn=FALSE;
    int pgrp, pid, res;

    int         c;
    extern char *optarg;
    
    while ((c = getopt(argc, argv, "vhnrc:")) != EOF)
    {
        switch (c)
        {
            case 'c':
                configfile = optarg;
                break;
            case 'n':
                daemon = FALSE;
                break;
            case 'r':
                respawn = TRUE;
                break;
            case 'v':
                printf("%s\n\%s\n", version, source);
                break;
            case 'h':
                usage(argv[0]);
                exit(0);
	}
    }

    if (! configfile) {
	usage(argv[0]);
	exit(-1);
    } 

    /* initial default logging, changed later when  config loaded */

    exception_init(4, NULL);
    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO,"cSnmpCron %s", version);
    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO,"cSnmpCron %s", source);

    memset(targetfilepath,0,sizeof(targetfilepath));
 
    joblist = init_config(configfile, &config, NULL);
    if (! joblist) {
        exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "Nothing to do");
	exit(0);
    }
    if (respawn==TRUE)
	config.respawn=1;
    else
	config.respawn=0;

    exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Starting");
 

    if (chdir("/var/log/snmp")!=0) {		// want core file 
	fprintf(stderr, "Fatal. Failed to change dir to /var/log/snmp\n");
	exit(-1);
    }

    /* Daemon init stuff */

    if (daemon==TRUE) {
        if ((pid=fork())>0) 
            exit(0);
        else if (pid == -1) {
            fprintf(stderr,"Failed to fork child\n");
            exit(-1);
        }

        if ((pgrp = setsid()) == -1) {
            fprintf(stderr,"Failed to become group leader\n");
            exit(-1);
        }

        if (freopen("/dev/null","r",stdin) == NULL) {
            fprintf(stderr,"Failed to reopen stdin to devnull\n");
            exit(-1);
        }
        if (freopen("/dev/null","w",stdout) == NULL) {
            fprintf(stderr,"Failed to reopen stdout to devnull\n");
            exit(-1);
        }
    }

    res = scheduler(&config, joblist);

    freejoblist(joblist);
    free_cS_config(&config);

    exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Done");

    /* Attempt at crash bug workaround */
    if ((respawn==TRUE) && (res==2)) {
        exception(MODULE_NAME, EXCEPTION_PRIORITY_INFO, "Respawning");
	execv(argv[0], argv);	
    }
}


void usage(char *name)
{
    fprintf(stderr,"%s [-n] [-r] [-h] [-v] -c configfile\n", name);
    fprintf(stderr,"\t-c configfile = Where Configfile contains target lists and thread limits.\n");
    fprintf(stderr,"\t-n = Don't run as daemon.\n");
    fprintf(stderr,"\t-r = respawn daily. Bugs in snmplib workaround\n");
    fprintf(stderr,"\t-h = this help.\n");
    fprintf(stderr,"\t-v = version string.\n");
}
