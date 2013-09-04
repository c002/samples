/*** 
 * Loads the [main] section of the config file
 *
 * 
 * 
 *
 */

#include <common.h>
#include <iniparser.h>

#define MODULE_NAME "config.c"

static char version[] ="

config_t *load_config(dictionary *dict, config_t *config)
{


    config->loglevel = iniparser_getint(dict, "main:loglevel", 5);
    config->maxworkers = iniparser_getint(dict, "main:maxworkers", 12);
    config->maxload = iniparser_getint(dict, "main:maxload", 5);
    config->roll = iniparser_getint(dict, "main:roll", 0);

    config->separator = iniparser_getstring(dict, "main:seperator", NULL);
    if (!config->separator) config->separator=strdup(" ::: ");

    config->datadir = iniparser_getstring(dict, "main:datadir", NULL);
    if (!config->datadir)  config->datadir=strdup("/var/log/");

    config->community = (u_char *) iniparser_getstring(dict, "main:community", NULL);
    if (!config->community) config->community=(u_char *) strdup("public");

    config->modulebase = iniparser_getstring(dict, "main:modulebase", NULL);
    if (!config->modulebase) config->modulebase=strdup("/opt/snmp/lib");

    config->logfile = iniparser_getstring(dict, "main:logfile", NULL);

    return(config);
}

void free_cS_config(config_t *config)
{
    free(config->community);
    free(config->modulebase);
    free(config->logfile);
    free(config->datadir);
    free(config->separator);

    return;
}

