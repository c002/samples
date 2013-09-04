#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <assert.h>

#define TRUE 1
#define FALSE 0

#ifndef ulong
#define ulong unsigned long
#endif

#define MAX_THREADS 101

typedef int bool;

typedef struct config_struct {
    int maxworkers;	// Max Number of worker threads
    int maxload;	// not implemented
    int respawn;	// restart daemon.
    int loglevel;	// What to log
    int roll;		// to roll the log file or not
    char *modulebase;	// dir where modules are
    u_char *community;	// SNMP community
    char *logfile;	// the logfile path 
    char *separator;	//data column sperator string
    char *datadir;	// dir to store results in
} config_t;

typedef struct charlist_struct
{
    char *str;
    struct charlist_struct *next;
} charlist_t;
    
#include <exception.h>
#include <iniparser.h>
#include <util.h>

config_t *load_config(dictionary *dict, config_t *config);
void free_cS_config(config_t *config);

#endif
