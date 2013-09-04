#include <stdio.h>
#include <unistd.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>


#include <module.h>

int spread_run(char *target, config_t *config, joblist_t *job, taskresult_t *taskresult);

main()
{

    spread_run("fred", NULL, NULL, NULL);

}
