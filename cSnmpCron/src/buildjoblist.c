/*
 * 
 * Load an ini style config using the iniparser lib
 * bung this into our structures.
 *
 * 
 * 
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <iniparser.h>

#include <dlfcn.h>
#include <link.h>

#include <common.h>
#include <job.h>
#include <util.h>

#define MODULE_NAME	"buildjoblist.c"
static char version[] ="

extern int loglevel;

extern char *reerrstr;
extern int reerror;

void dump_dict(dictionary *d);
charlist_t *iniparser_keys(dictionary * d, char *section);
void freejob(joblist_t *job);
void freekeys(charlist_t *list);

int BUFSIZE=1024;

//joblist_t *BuildJoblist(char *fname)
joblist_t *BuildJoblist(config_t *config, dictionary *dict, joblist_t *badjob)
{
    //dictionary *dict;
    char *k, **res, *section, *token, *val;
    charlist_t *keys;
    char entry[BUFSIZE+1];
    char module[BUFSIZE+1];
    int jobid=1;
    int nsect;
    int i=0, n=0, badjobid=-1;
    joblist_t *joblist=NULL, *joblist_head=NULL;

    if (badjob)
	badjobid=badjob->id;

    // dict = iniparser_load(fname);

    /* Get all the sections */
    nsect = iniparser_getnsec(dict);
    for (n=0; n<nsect; n++) {		// Each Section

	section = iniparser_getsecname(dict, n);
	if (strcmp(section, "main")==0) {
		// Set global parameters
		continue;
	}

	if (! section ) exit(-1);

	if (!joblist) {
            if (! (joblist = (joblist_t *) malloc( sizeof(joblist_t))) ) {
                exception(MODULE_NAME, EXCEPTION_PRIORITY_FATAL, "Failed to allocate memory");
		return(NULL);
	    }
            joblist_head = joblist;
	} else {
            if (! (joblist->next = (joblist_t *) malloc( sizeof(joblist_t))) ) {
                exception(MODULE_NAME, EXCEPTION_PRIORITY_FATAL, "Failed to allocate memory");
		return(NULL);
	    }
            joblist= joblist->next;
	}

	joblist->valid=TRUE;
        joblist->id=0;
        joblist->timestamp=0;
        joblist->activecount=0;
	joblist->prefix=NULL;
        joblist->jobname=strdup(section);
        joblist->modname=NULL;
        joblist->modpath=NULL;
        joblist->modhandle=NULL;
        joblist->runmin=NULL;
        joblist->runhr=NULL;
        joblist->targetlist[0]=NULL;
        joblist->next=NULL;
	
	snprintf(entry,BUFSIZE, "%s:%s", section, CONF_PREFIX);
	val = iniparser_getstring(dict, entry, "null");

	if (strncmp(val, "null", strlen("null"))==0) {
	    joblist->valid=FALSE;
 	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "Section=%s, key not found %s",section,CONF_PREFIX);
	} else {
 	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Section=%s, Key=%s, Val=%s",section, CONF_PREFIX, val);
            joblist->prefix=strdup(val);
	}

	snprintf(entry,BUFSIZE, "%s:%s", section, CONF_RUNTIME);
	val = iniparser_getstring(dict, entry, "null");

	if (strncmp(val, "null", strlen("null"))==0) {
	    joblist->valid=FALSE;
 	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "Section=%s, key not found %s",section,CONF_RUNTIME);
	} else {
	    res = rematch(CONF_RUNPAT, val);
	    if (!res) {
 	        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "Section=%s, Key=%s, incorrect runtime format",section, CONF_RUNTIME);
	        joblist->valid=FALSE;
	    } else {
		if (badjob && (jobid == badjob->id))
		    joblist->valid=FALSE;
                joblist->id=jobid++;
                joblist->runmin=strdup(res[1]);	// group 1
                joblist->runhr=strdup(res[2]);	// group 2
		for (i=0; res[i]!=NULL;i++)
		    free(res[i]);
 	        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Section=%s, min=%s hr=%s",section, joblist->runmin, joblist->runhr);
	    }
	}
	
	snprintf(entry,BUFSIZE, "%s:%s", section, CONF_TARGETS);
	// printf("Targets=%s\n", entry);
	val = iniparser_getstring(dict, entry, "null");

	if (strncmp(val, "null", strlen("null"))==0) {
 	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "Section=%s, key not found %s",section,CONF_TARGETS);
	    joblist->valid=FALSE;
	    continue;
	}

	/* Can load targetlist by file also */

	if ( strncmp(val,TARGET_FROM_FILE,strlen(TARGET_FROM_FILE))==0) {
	    if (joblist->targetlist[0]) free_targetlist(joblist->targetlist);
	    if (!( load_targets_from_file(val+strlen(TARGET_FROM_FILE), joblist->targetlist))) {
 	        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "Section=%s, No targets found in file: %s",section, val+strlen(TARGET_FROM_FILE));
		continue;
	    }
	} else {
	    i=0;
	    token=strtok(val,", ");
	    while (token) {
                joblist->targetlist[i++] = strdup(token);
	        token=strtok(NULL,", ");
	    }
            joblist->targetlist[i] = NULL;
	}

	snprintf(entry,BUFSIZE, "%s:%s", section, CONF_MODNAME);
	val = iniparser_getstring(dict, entry, "null");
	if ( strncmp(val,"null",strlen("null"))==0) {
	    joblist->valid=FALSE;
 	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Section=%s, Missing module name %s",section, CONF_MODNAME);
 	} else
            joblist->modname = strdup(val);

	snprintf(entry,BUFSIZE, "%s:%s", section, CONF_MODULE);
	val = iniparser_getstring(dict, entry, "null");

	memset(module,0, sizeof(module));

	if ( strncmp(val,"null",strlen("null"))!=0) {
	    snprintf(module, sizeof(module), "%s/%s", config->modulebase, val); 
 	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Section=%s, Opening module %s",section, module);

            joblist->modpath = strdup(module);
            joblist->modhandle = dlopen(module, RTLD_LAZY | RTLD_GLOBAL);

            if (! joblist->modhandle) {
	        joblist->valid=FALSE;
                // fprintf(stderr, "Oops, %s\n", dlerror());
 	        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "Section=%s, %s",section, dlerror());
	    } else {
 	        va_exception(MODULE_NAME, EXCEPTION_PRIORITY_DEBUG, "Section=%s, Module %s succesfully loaded", section, val);
	    }
	} else {
	    joblist->valid=FALSE;
 	    va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "Section=%s, key not found %s",section,CONF_MODULE);
	}


    }

    return ( remove_invalid_jobs(joblist_head));

    // return (joblist_head);
}

joblist_t *remove_invalid_jobs(joblist_t *joblist)
{

    joblist_t *job, *head=NULL, *nextjob=NULL, *tmp=NULL;
 
    job=joblist; 
    tmp=joblist;

    while (job) {
	if (!job->valid) {
            va_exception(MODULE_NAME, EXCEPTION_PRIORITY_WARNING, "Removing invalid job [%d], see previous error message", job->id);
	    tmp=job; 
	    job=job->next;
	    freejob(tmp);
	} else {
	    if (! head) head=job;
	    nextjob = job->next;
	    while (nextjob) {
	        if (! nextjob->valid) {
		    job->next=nextjob->next;
		    freejob(nextjob);
		    nextjob=job->next;
	        } else {
		    job=nextjob;
		    nextjob=nextjob->next;
		}
	    }
	    job=nextjob;
	}
    }
    return(head);
}

void freejob(joblist_t *job)
{
	int i;

	if (job->prefix) free(job->prefix);
        if (job->jobname) free(job->jobname);
        if (job->modname) free(job->modname);
     //   if (job->modhandle) free(job->modhandle);
	if (job->modhandle) dlclose(job->modhandle);
	if (job->modpath) free(job->modpath);
        if (job->runmin) free(job->runmin);
        if (job->runhr) free(job->runhr);
	for (i=0; job->targetlist[i]!=NULL; i++)
	    free(job->targetlist[i]);

	free(job);
}

/* initparser has its own dumper, but need to test
 * the iniparser_keys() function
 */
void dump_dict(dictionary *dict)
{
 
    char entry[1024];
    char *section, *val;
    int result=0;
    int sect, nsect, n;
    charlist_t *k;
    charlist_t *keys;	

    nsect = iniparser_getnsec(dict);

    for (n=0; n<nsect; n++) {
        section = iniparser_getsecname(dict, n);
	printf("Section %d=%s\n", n, section);
        keys = iniparser_keys(dict, section);

        for (k=keys; k!=NULL; k=k->next) {
	    sprintf(entry,"%s:%s", section, k->str);
    	    val = iniparser_getstring(dict, entry, "null");
	    printf("  key=%s, val=%s\n",k->str, val);
        }
	freekeys(keys);
    }
}
/*
 * plucks out the keys from a given section.  
 * Missing from the iniparser lib
 */

charlist_t *iniparser_keys(dictionary * d, char *section)
{

    char keym[1024], *val;
    charlist_t *klist=NULL, *head=NULL;
    int j, seclen;

    if (section) {
        sprintf(keym, "%s:", section);

        for (j=0 ; j<d->size ; j++) {
            if (d->key[j]==NULL)
                continue ;
	    seclen=strlen(section)+1;
            if (strncmp(d->key[j], keym, seclen)==0) {
	        if (klist==NULL) {
    		    klist = (charlist_t *) malloc(sizeof(charlist_t));
		    klist->next=NULL;
    		    head=klist;
		} else {
    		    klist->next = (charlist_t *) malloc(sizeof(charlist_t));
		    klist=klist->next;
		    klist->next=NULL;
		}
		klist->str = strdup(d->key[j]+seclen);
	    }
	}
    }

    return head;
}

void freekeys(charlist_t *list)
{
    charlist_t *k;
    for (k=list;k!=NULL;k=k->next) {
	    if (k->str) free(k->str);
	    free(k);
    }
}


void dump_joblist(joblist_t *joblist)
{

    joblist_t *job;
    char *t;

    for (job=joblist; job!=NULL; job=job->next)
    {
	int i=0;
        if ( !job)  
	    fprintf(stderr,"job is null\n");
	fflush(stderr);
	printf("Job [%d] ", job->id);fflush(stdout);
	if (job && job->jobname) printf("%s is %s\n", job->jobname, (job->valid ? "Valid" : "Not Valid")); fflush(stdout);
	if (job->prefix) printf("\tPrefix=%s\n", job->prefix); fflush(stdout);
	if (job->modname) printf("\tmodname=%s\n", job->modname);
	printf("\tModule Loading: %s\n", (job->modhandle ? "Success" : "Error" ) );
	if (job->runmin) printf("\tMinutes=%s\n", job->runmin);
	if (job->runhr) printf("\tHours=%s\n", job->runhr);
	for (i=0; job->targetlist[i]!=NULL; i++)
	    printf("\ttarget=%s\n", job->targetlist[i]);
    }


    return;
}
