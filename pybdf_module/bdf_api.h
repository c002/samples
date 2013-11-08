#ifndef _BDF_API_H

#define _BDF_API_H

#include "bdf_file.h"

#define MAX_TAGS	1024
#define MAX_ALTTAGS	32
#define BLOCKSIZE	4

static struct summarydetails {
    uint64_t total_to_bytes;            /* Total Number of To_Bytes */
    uint64_t total_from_bytes;    	/* Total Number of From_Bytes */
    uint32_t count;    			/* Number of flows of this type */
} SUMMARYDETAILS;

static struct bdf_headers {
    bdf_file_header *head;
    bdf_file_tag_header *taghead;
    bdf_file_alttag_header *alttaghead;
/*    bdf_file_alttag *alttag; */
    char **tagdic;
    char **alttagdic;
} BDF_HEADERS;

typedef struct 
{
    bdf_file_data block[BLOCKSIZE];
} bdf_file_block;


int bdf_summary(FILE *fp, struct summarydetails *sum_struct);
long bdf_flowcount(FILE *f1);
int bdf_readflow(FILE *f1, bdf_file_data *data);
int bdf_readhead(FILE *f1, struct bdf_headers *bh);
int free_bdf_headers(struct bdf_headers *bh);
char * bdf_tagname(int tagid, struct bdf_headers *bh);
char * bdf_alttagname(int alttag, struct bdf_headers *bh);
char *ip2str(unsigned long ip, char *str);
int bdf_readxflow(FILE *f1, bdf_file_block *block, int blocksize);
char *bdf_errlookup(int err);

#endif
