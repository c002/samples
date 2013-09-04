/* clean()
 *
 * 
 * 
 */

#include <stdio.h>
#include <unistd.h>

#include <module.h>

int clean_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult);

main()
{

    clean_run("fred", NULL, NULL, NULL);

}

int clean_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult)
{

    FILE *f;
    char *localname="cleanmodule";
    char fname[127];
    char buf[127];
    int counter=0;

    if (!target) return(0);

    printf(" -- [clean_run] --\n");

    memset(buf, 0, sizeof(buf));

    sprintf(fname, "/var/log/snmp");
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
