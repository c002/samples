/* spread()
 *	Test out the thread engines fairness at competing
 *	evenly for available threads.  We want jobs that run
 *	at the same to have an even chance of running.
 *
 * 
 * 
 */

#include <stdio.h>
#include <unistd.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>


#include <module.h>

int spread_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult);

int spread_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult)
{

    FILE *f;
    char *localname="testmodule";
    char fname[127];
    char buf[127];
    int counter=0;

    if (!target) return(0);

    printf(" -- [module_run(%s)] --\n", target);

    memset(buf, 0, sizeof(buf));

    sprintf(fname, "/tmp/%s", target);
    if ( f=fopen(fname,"r")) {
        lockf(fileno(f), F_LOCK, 0);
        fgets(buf, sizeof(buf), f); 
        fclose(f);
    }

    if (! (f=fopen(fname,"w")) ) {
	perror("fopen w");
	exit(-1);
    }
    
    // fprintf(stderr,"[%s]%d, %d\n", buf, strlen(buf), sizeof(buf));
    if (strlen(buf)==0)
	counter=1;
    else
        counter = atoi(buf)+1;

    fprintf(f,"%d\n", counter);
    lockf(fileno(f), F_ULOCK, 0);
    fclose(f);

/*
    printf(" -- [module_run(), sleep(1), target=%s, datadir=%s, logfile=%s] --\n", 
                target,config->datadir, config->logfile);
*/
    // sleep(5);
    return(0); 
}
