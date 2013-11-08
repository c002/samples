/*  Swig defs for bdf_api.c to build a python module */

%module bdf_api
%{
#include <stdio.h>
#include <bdf_api.h>
%}

#define api_version       "1.000"
#define BDF_FILE_MAX_TAG_LENGTH 32
#define BDF_FILE_MAX_ALTTAG_LENGTH 32
#define MAX_TAGS        1024
#define BLOCKSIZE       4

typedef long 		time_t;
typedef unsigned char   uint8_t;
typedef unsigned int    uint32_t;
typedef unsigned long long    uint64_t;
typedef unsigned short  uint16_t;

typedef struct {
    uint32_t  type; 
    uint32_t  version;
    uint32_t  file_size;       /* Length of the entire file (incl headers) */
    uint32_t  collector_ip;    /* IP address of this collector */
    time_t   checkpoint_time; /* Time the file was written */
    time_t   time_start;      /* Time of the first data item */
    time_t   time_end;        /* Time of the last data item */
    uint32_t  rows;            /* Num of rows in the DATA block */
}   bdf_file_header;

typedef struct
{
    uint32_t  type;            /* Type of this block (BDF_FILE_TAG_TABLE) */
    uint16_t   rows;            /* Num of tags in the TAGS block */
} bdf_file_tag_header;

typedef struct {
    uint32_t    type;
    uint16_t    rows;
} bdf_file_alttag_header;

typedef struct {
    char        alttag[BDF_FILE_MAX_ALTTAG_LENGTH];
    uint16_t    id;
} bdf_file_alttag;

typedef struct
{
    uint32_t    ip_addr;
    uint32_t    ip_proxy;
    uint64_t    bytes_to;
    uint64_t    bytes_from;
    uint16_t    tag_id;
    uint16_t    aas;
    uint32_t     alt_tags;      /* limited to 32 bits here.. */
} bdf_file_data;

typedef struct 
{
    bdf_file_data block[BLOCKSIZE];
} bdf_file_block;

static struct bdf_headers {
    bdf_file_header *head;
    bdf_file_tag_header *taghead;
    bdf_file_alttag_header *alttaghead;
    bdf_file_alttag *alttag;
    char **tagdic;
    char **alttagdic;
} BDF_HEADERS;

static struct summarydetails {
    uint64_t total_to_bytes;            /* Total Number of To_Bytes */
    uint64_t total_from_bytes;          /* Total Number of From_Bytes */
    uint32_t count;                     /* Number of flows of this type */
} SUMMARYDETAILS;

%{
/*
 * Helper func for python module to return the name of tagid
 *
 */
char * bdf_tagname(int tagid, struct bdf_headers *bh) {

    char *str;

    if (!bh || !bh->tagdic) 
        return(NULL);

    return(bh->tagdic[tagid]); 

}

/*
 * Helper func for python module to return the name of alttagid
 * Alttag is a bitmask of alttags.  Returns the alttagname of the highest
 * bit givin.
 */
char * bdf_alttagname(int alttag, struct bdf_headers *bh) {
 
    unsigned int mask=1<<31;
    int found=0, index=32;

    if (!bh || !bh->alttagdic) 
        return(NULL);

    while ((!found) && (mask>0)) {
        found = alttag & mask;
        mask=mask>>1;
        index--;
    }
      
    return(bh->alttagdic[index]);
    
}

bdf_file_data bdf_getdata(bdf_file_block *block, int index)
{

    return(block->block[index]);
}

int bdf_fastread(FILE *f1, bdf_file_data *data)
{
    int n;

    if (! data)
        return(-1);
    if (!(n=fread(data, sizeof(bdf_file_data), 1, f1))) {
       return(0); 
    }

    return 1;
}

%}


/* Mapping of standard FILE io */
FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *);


int bdf_readhead(FILE *, struct bdf_headers *);
int bdf_readflow(FILE *, bdf_file_data *);
int bdf_readxflow(FILE *, bdf_file_block *, int );
long bdf_flowcount(FILE *);
int bdf_summary(FILE *, struct summarydetails *);
char * bdf_tagname(int , struct bdf_headers *);
char * bdf_alttagname(int , struct bdf_headers *);
bdf_file_data bdf_getdata(bdf_file_block *, int );
int bdf_fastread(FILE *, bdf_file_data *);
