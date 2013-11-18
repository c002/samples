#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "btalib.h"
#include "bgp_api.h"
#include "ip2cidr.h"
#include "btalib.h"

void print_attr(cidr_userdata_t* userdata);

int main(int argc, char** argv)
{
    char*		str;
    char*		prefixstr;
    char*		suffixstr;
    uint32_t		prefix;
    uint32_t		length;
    cidr_userdata_t	userdata;
    
    bgp_debug_level = 10;
    
    bgp_db_init(argv[0]);
    
    str = strdup(argv[1]);
    prefixstr = strtok(str, "/");
    suffixstr = strtok(NULL, "/");
    
    prefix = ip_str2addr(prefixstr);
    length = atoi(suffixstr);
    
    userdata.prefix = prefix;
    userdata.length = length;
    
    printf("   Network              Next Hop             Path                (-- communities) \n");
    
    cidr_lookup_tree(prefix, length, print_attr);
    
    return 0;
} /* main */

void print_attr(cidr_userdata_t* userdata) {
    unsigned int	i;
    struct bgp_attr*	attr;
    static char		charbuf1[128];
    static char		charbuf2[128];
    
    if(userdata == NULL || userdata->datap == NULL || userdata->size != sizeof(struct bgp_attr)) {
	printf("error in data\n");
    }
    
    attr = userdata->datap;
    
    sprintf(charbuf1, "%s/%d", ip_addr2str(userdata->prefix), userdata->length);
    sprintf(charbuf2, "%s", ip_addr2str(attr->nexthop));
    
    printf("*> %-20s %-19s", charbuf1, charbuf2);
    for(i = 0 ; i < attr->aspath_len ; i++) {
	printf("%d", (unsigned short) attr->aspath[i]);
	if(i < attr->aspath_len - 1) {
	    printf(" ");
	}
    }
    
    switch(attr->origin) {
    case bgp_ORIGIN_IGP:
	printf(" i");
	break;
    case bgp_ORIGIN_EGP:
	printf(" e");
	break;
    default:
	printf(" ?");
    }
    
    if(attr->communities_len) {
	printf(" -- ");
	for(i = 0 ; i < attr->communities_len ; i++) {
	    printf("%d:%d ", (attr->communities[i] & 0xFFFF0000) >> 16, attr->communities[i] & 0x0000FFFF);
	}
    }
    printf("\n");
}
