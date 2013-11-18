#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "ip2cidr.h"
#include "mpmalloc.h"

int main() {
    char *command;
    char *p_str;
    char *desc;
    int a, b, c, d;
    uint32_t p, lp;
    int s, ls;
    char line[80];
    int gotlook = 0;
    int inserted = 0;
    int updated = 0;
    int lines = 0;
    cidr_userdata_t	userdata;
    char	buf[1024];
    
    printf("\nInitialising...");
    fflush(stdout);
    if (cidr_init("iptest",0) == -1)
      {
	printf("cidr_init failed. Exiting\n");
	exit(1);
      }
    printf("done.\n\n");

    while(gets(line) != NULL) {
	desc=NULL;
	command = line;
	lines++;
	while(*command && isspace((int)*command)) command++;
	p_str = command;
	while(*p_str && !isspace((int)*p_str)) p_str++;
	*p_str++ = '\0';
	while(*p_str && isspace((int)*p_str)) p_str++;
	/*
	s_str = p_str;
	while(*s_str && !isspace((int)*s_str)) s_str++;
	*s_str++ = '\0';
	while(*s_str && isspace((int)*s_str)) s_str++;
	*/
	desc = p_str;
	while(*desc && !isspace((int)*desc)) desc++;
	*desc++ = '\0';
	while(*desc && isspace((int)*desc)) desc++;

	a = b = c = d = 0;
	sscanf(p_str, "%d.%d.%d.%d/%d", &a, &b, &c, &d, &s);
	p = a<<24 | b<<16 | c<<8 | d;
	/*
	printf("%x %d\n", p, s);
	*/

	if(!strcmp(command, "LUP")) {
	    if(!gotlook) {
		printf("Starting lookups\n");
		gotlook = 1;
	    }
	    lp = p;
	    ls = s;
	    userdata.prefix = lp;
	    userdata.length = ls;
	    userdata.datap = buf;
	    userdata.size = sizeof(buf);
	    if(cidr_lookup(&userdata) != 0) {
		printf("ERROR - lookup failed on %s\n", p_str);
	    }
	    lp = userdata.prefix;
	    ls = userdata.length;
	    if(lp == 0) {
		printf("No matching networks\n\n");
	    } else {
		desc = userdata.datap;

		printf("%d.%d.%d.%d/%d - %s\n\n", (int)(lp>>24)&0xff, (int)(lp>>16)&0xff, (int)(lp>>8)&0xff, (int)lp&0xff, ls, desc);

	    }
	    continue;
	}
	if(!strcmp(command, "ADD")) {
	    userdata.datap = desc;
	    userdata.size = strlen(desc) + 1;
	    userdata.prefix = p;
	    userdata.length = s;
	    if(cidr_insert(&userdata, NULL) == 0) {
		inserted++;
	    } else {
		printf("cidr_add for %s failed\n", p_str);
	    }
	    continue;
	}
	if(!strcmp(command, "UPD")) {
	    userdata.datap = desc;
	    userdata.size = strlen(desc) + 1;
	    userdata.prefix = p;
	    userdata.length = s;
	    if(cidr_update(&userdata, NULL) == 0) {
		updated++;
	    } else {
		printf("cidr_update for %s failed\n", p_str);
	    }
	    continue;
	}
	
	
	if(!strcmp(command, "DEL")) {
	    if(cidr_remove(p, s, NULL) != 0) {
		printf("cidr_remove failed for %s\n", p_str);
	    }
	    continue;
	}

	if(!strcmp(command, "TRE")) {
	    cidr_lookup_tree(p, s, (void*)1);
	    continue;
	}
	
	if(!strcmp(command, "PRT")) {
	    cidr_printtree();
	    continue;
	}

	if(!strcmp(command, "CHK")) {
	    cidr_checktree();
	    continue;
	}

	if(!strcmp(command, "EXP")) {
	    cidr_expireall();
	    continue;
	}
	
	if(!strcmp(command, "PUR")) {
	    cidr_purge(NULL);
	    continue;
	}
	
	printf("Command = %s\n", command);
	printf("Prefix = %s\n", p_str);
	printf("Description = %s\n", desc);
	printf("\n");

    }

    printf("Inserted = %d\n", inserted);
    printf("Updated = %d\n", updated);
    printf("Lines = %d\n", lines);
    
    return 1;
} /* main */

