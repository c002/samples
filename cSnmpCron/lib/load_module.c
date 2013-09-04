/* 
 * Load a shared lib object
 *
 * $Source: /usr/local/cvsroot/cSnmpCron/lib/load_module.c,v $
 * $$id$
 */

#include <stdio.h>
#include <signal.h>
#include <dlfcn.h>
#include <link.h>

int load_module(char *fname);

/* sighandler
    if SIGHUP, reload module
*/
int load_module(char *fname)
{

    void *handle;
    int  (*fptr)(char *);
    char *modname="module";
    char run[1024];
    char store[1024];
    char module[1024];
    char *path="./";

    // daemon();

    snprintf(run, strlen(modname)+5,"%s_run", modname);
    snprintf(store, strlen(modname)+7, "%s_store", modname);

    snprintf(module, strlen(modname)+strlen(path)+4, "%s%s.so", path, modname);

    printf("module=%s, f1=%s, f2=%s\n", module, run, store); fflush(stdout);

    handle = dlopen(module, RTLD_LAZY | RTLD_GLOBAL);
    if (! handle) {
	fprintf(stderr, "%s\n", dlerror());
	exit(-1);
    }

    fptr = (int (*)(int))dlsym(handle, run);
    fptr2 = (int (*)(int))dlsym(handle, store);
    // iptr = (int *)dlsym(handle, "my_object");


    /* See of module has a run() function */
    if ((fptr = (int (*)())dlsym(RTLD_DEFAULT, run)) != NULL) {
              (*fptr)( "test run");
    } else 
	fprintf(stderr,"Error , function not found\n");

    if ((fptr2 = (int (*)())dlsym(RTLD_DEFAULT, store)) != NULL) { 
	(*fptr2)( "test store");
    } else 
	fprintf(stderr,"Error , function not found\n");

    //dldump();
    // dlinfo();

    dlclose(handle);

}
