/*
 *
 */
#include <stdio.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>


#include <module.h>

int module_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult);

int module_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult)
{

    FILE *f;
    char *localname="testmodule";

    if (!target) return(0);

    printf(" -- [module_run(%s)] --\n", target);
/*
    printf(" -- [module_run(), sleep(1), target=%s, datadir=%s, logfile=%s] --\n", 
                target,config->datadir, config->logfile);
*/
    return(0); 
}
