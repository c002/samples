#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "bgp_api.h"
#include "ip2cidr.h"
#include "btalib.h"

/* a hack of slf's iptest.c */

int main(int argc, char** argv) {
    char*		command;
    char*		p_str;
    char*		s_str;
    char*		desc;
    int			a, b, c, d;
    char		line[80];
    int			gotlook = 0;
    int			inserted = 0;
    int			lines = 0;
    unsigned int	i;
    int			retval;
    
    uint32_t			prefix;
    uint32_t			length;
    struct bgp_route_info	route;
    struct bgp_attr		attr;
    
    bgp_debug_level = 10;
    
    printf("\nInitialising...");
    fflush(stdout);
 
/*
    retval = bgp_db_init(argv[0]);
*/
    retval = bgp_db_attach(argv[0]);
    if (retval==1)
	fprintf(stderr,"\nAttach existing segment\n");
    else if (retval==0) 
	fprintf(stderr,"New segment\n");
    else {
	fprintf(stderr,"Failed to attach to existing bgp_db\n");
	exit(-1);
    }

    printf("done.\n\n");
    
    while(gets(line) != NULL) {
	command = line;
	lines++;
	while(*command && isspace((int)*command)) command++;
	p_str = command;
	while(*p_str && !isspace((int)*p_str)) p_str++;
	*p_str++ = '\0';
	while(*p_str && isspace((int)*p_str)) p_str++;
	s_str = p_str;
	while(*s_str && !isspace((int)*s_str)) s_str++;
	*s_str++ = '\0';
	while(*s_str && isspace((int)*s_str)) s_str++;
	desc = s_str;
	while(*desc && !isspace((int)*desc)) desc++;
	*desc++ = '\0';
	while(*desc && isspace((int)*desc)) desc++;
	
	a = b = c = d = 0;
	sscanf(p_str, "%d.%d.%d.%d", &a, &b, &c, &d);
	prefix = a<<24 | b<<16 | c<<8 | d;
	length = atoi(s_str);
	/*
	  printf("%x %d\n", p, s);
	*/

	if(!strcmp(command, "LUP")) {
	    if(!gotlook) {
		printf("Starting lookups\n");
		gotlook = 1;
	    }
	    printf("Looking up %s/%s\n", p_str, s_str);
	    
	    route.prefix = prefix;
	    route.length = length;
	    route.attr = &attr;
	    retval = bgp_lookup_route(&route);
	    
	    if(retval == 0) {
		printf("No matching networks\n\n");
	    } else {
		printf("%s/%d, ", ip_addr2str(prefix), length);
		printf("nexthop=%s, ", ip_addr2str(route.attr->nexthop));
		if(route.attr->aspath_len) {
		    printf("AS_PATH=");
		    for(i = 0 ; i < route.attr->aspath_len ; i++) {
			printf(" AS%d", (uint16_t) route.attr->aspath[i]);
		    }
		}
		if(route.attr->communities_len) {
		    printf("communities=");
		    for(i = 0 ; i < route.attr->communities_len ; i++) {
			printf("0x%08x ", (unsigned int) route.attr->communities[i]);
		    }
		}
		printf("\n");
	    }
	    continue;
	}
	if(!strcmp(command, "ADD")) {
	    printf("add command not supported\n");
	    continue;
	}
	if(!strcmp(command, "DEL")) {
	    printf("del command not supported yet\n");
	    continue;
	}
	/*
	printf("Command = %s\n", command);
	printf("Prefix = %s\n", p_str);
	printf("Suffix = %s\n", s_str);
	printf("Description = %s\n", desc);
	printf("\n");
	*/
    }
    
    printf("Inserted = %d\n", inserted);
    printf("Lines = %d\n", lines);
    
    return 0;
} /* main */

