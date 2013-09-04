/*
 *
 */
#include <stdio.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <module.h>

int module3_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult)
{
    sleep(0);
    return(0); 
}
