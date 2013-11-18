#include <sys/types.h>
#include <stdio.h>
#include <synch.h>
#include <ctype.h>
#include <stdlib.h>

#include "bgp_api.h"

/* a hack of slf's iptest.c */

int main()
{
    int octet1, octet2;
    char *command;
    char *p_str;
    char *s_str;
    char *desc;
    int a, b, c, d;
    char line[80];
    int gotlook = 0;
    int inserted = 0;
    int lines = 0;
    int i;
    
    in_addr_t		addr;
    uint8_t		length;
    struct bgp_attr*	attr;
    
    bgp_debug_level = 0;
    
    printf("\nInitialising...");
    fflush(stdout);
    bgp_db_init();
    printf("done.\n\n");
    
    for(octet1 = 0; octet1 < 255; octet1++) {
	for(octet2 = 0; octet2 < 255; octet2++) {
	    a = 203;
	    b = octet1;
	    c = octet2;
	    d = 16;
	    addr = a<<24 | b<<16 | c<<8 | d;
	    length = 32;
	    
	    printf("%d.%d.%d.%d - ", a, b, c, d);
	    attr = bgp_lookup_route(&addr, &length);

	    if(attr == 0) {
		printf("No matching networks\n");
	    } else {
		printf("%s/%d, ", ip_addr2str(addr), length);
		printf("nexthop=%s, ", ip_addr2str(attr->nexthop));
		if(attr->aspath_n) {
		    printf("AS_PATH=");
		    for(i = 0 ; i < attr->aspath_n ; i++) {
			printf(" AS%d", (uint16_t) attr->aspath[i]);
		    }
		}
		if(attr->communities_n) {
		    printf("communities=");
		    for(i = 0 ; i < attr->communities_n ; i++) {
			printf("0x%08x ", attr->communities[i]);
		    }
		}
		printf("\n");
	    }
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

