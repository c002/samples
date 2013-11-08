/***
 * bdf_api
 *
 * An API for reading BDF files 
 * Accepts version 4.3, 4.4 and 5.0, although version 5.0 has not
 * been tested with this. (It came after 'sortbdf' which is standalone)
 *
 * Used in the snmpd netflow module, maybe a little imcomplete still.
 *
 * $Source: /usr/local/cvsroot/netflow/netflow/btalib/bdf_api.c,v $
 *
 ***/

#include <stdio.h>
#include <strings.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "bdf_api.h"

int free_tagdic(char **tagdic);			/* private func */
int free_alttagdic(char **alttagdic);		/* private func */

static int VERSION=4;				/* default bdf version */

/* Alttags are driven my bitmasks, this maps it to an arrayindex */
unsigned int ALTTAG_MAP[]= { 0x0000, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 
                           0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000,
                           0x00010000, 0x00020000, 0x00040000, 0x00080000, 
                           0x00100000, 0x00200000, 0x00400000, 0x00800000,
                           0x01000000, 0x02000000, 0x04000000, 0x08000000, 
                           0x10000000, 0x20000000, 0x40000000, 0x80000000};


static const char *error_str[] = { "0 = Ok",
                           "-1 = Error opening input file." ,
                           "-2 = Error reading header.",
                           "-3 = Error in version. 4.3 or 4.4 required.",
                           "-4 = Error reading tag dictionary.",
                           "-5 = Error mallocing tagtable.",
                           "-6 = Error mallocing alttag table.",
                           "-7 = Error mallocing bdf data table.",
                           "-8 = Error opening output file.",
                           "-9 = Error Unable to malloc filtered output segment.",
                           "-10 = Error opening log file.",
                           "-11 = Error copying flow to filtered segment.",
                           "-12 = Error removing bad output file."
                         };
/** 
 * bdf_flowcount()
 * 	Simple returns the number of flows in the given bdf file
 */
long bdf_flowcount(FILE *f1) 
{

    struct bdf_headers *bh;
    uint32_t count=0;
    int res;

    if (!(bh = (struct bdf_headers *) malloc(sizeof(struct bdf_headers))))  {
	return 0; 
    }
    
    if ( (res=bdf_readhead(f1, bh)) < 0) {
        free_bdf_headers(bh);
	return 0;
    }

    if (bh && bh->head )
        count=bh->head->rows;

    free_bdf_headers(bh);

    return (count);
}
/*
 * bdf_readhead()
 *    Reads the headers of a bdf file and stores these in 
 *    a bdf_headers structure.
 *
 */
int bdf_readhead(FILE *f1, struct bdf_headers *bh) 
{
    int major, minor;

    bdf_file_header *head=NULL;
    bdf_file_tag_header *tag_head=NULL;
    bdf_file_tag *bdf_tag=NULL;
    bdf_file_alttag_header *alttag_head=NULL;
    bdf_file_alttag *alttag=NULL;
    char **alttagdic=NULL;
    char **tagdic=NULL;

    int n, i;

    if (! bh) return(NULL);

    bh->head=NULL;
    bh->taghead=NULL;
    bh->tagdic=NULL;
    bh->alttaghead=NULL;
    bh->alttagdic=NULL;

    tagdic= (char **) calloc(MAX_TAGS,sizeof(char **));
    alttagdic= (char **) calloc(MAX_ALTTAGS,sizeof(char **));

    /* Seek to start */
    fseek(f1, 0,SEEK_SET);
/*
    for (i=0; i<=MAX_TAGS; i++) {
	tagdic[i]=NULL;
    }
    for (i=0; i<=MAX_ALTTAGS; i++) {
	alttagdic[i]=NULL;
    }
*/
    /* Read header */
    if (! ( head = (bdf_file_header *) malloc(sizeof(bdf_file_header))) ) {
	return(-1);
    }

    if (! (n=fread(head, sizeof(bdf_file_header), 1, f1))) {
	free(head);
        return(-2);
    }

    major=(head->version>>16) & 0xFFFF;
    minor=head->version & 0xFFFF;

    if (major == 5 && minor == 0)
	VERSION=5;
    else if (major !=4 || (minor!=3 && minor!=4))  {
	free(head);
	return(-3);
    }

    /* Read the tags dictionary */      
    if (! ( tag_head = (bdf_file_tag_header *) malloc(sizeof(bdf_file_tag_header))) ) {
	free(head);
	return(-4);
    }
    if (! (n=fread(tag_head, sizeof(bdf_file_tag_header), 1, f1))) {
	free(tag_head);
	free(head);
        return(-5);
    }

    if (! ( bdf_tag = (bdf_file_tag *) malloc(sizeof(bdf_file_tag))) ) {
	    free(tag_head);
	    free(head);
	    return(-6);
    }
    for (i=0; i< tag_head->rows;i++) {
        n=fread(bdf_tag, sizeof(bdf_file_tag), 1, f1);

/*	printf("DEBUG: %s\n", bdf_tag->tag); fflush(stdout); */

	tagdic[bdf_tag->id] = (char *) malloc(BDF_FILE_MAX_TAG_LENGTH+1);
	strcpy(tagdic[bdf_tag->id], bdf_tag->tag);

	if (! tagdic[bdf_tag->id]) {
		free(bdf_tag);
		free(tag_head);
		free(head);
		return(-9);
	}

    }
    free(bdf_tag);

    if (! ( alttag_head = (bdf_file_alttag_header *) malloc(sizeof(bdf_file_alttag_header))) ) {
	free(tag_head);
	free(head);
	return(-7);
    }
    if ( ! (n=fread(alttag_head, sizeof(bdf_file_alttag_header), 1, f1))) {
	free(tag_head);
	free(head);
	return(-10);
    }

    /* For number of alttags, read */
    if (! ( alttag = (bdf_file_alttag *) malloc(sizeof(bdf_file_alttag))) ) {
	    free(alttag_head);
	    free(tag_head);
	    free(head);
	    return(-8);
    }
    if (! alttag_head->rows || alttag_head->rows>MAX_ALTTAGS) {
	printf("*Barf* 170\n"); fflush(stdout);
    }
    for (i=0; i< alttag_head->rows;i++) {
	if (! (n=fread(alttag, sizeof(bdf_file_alttag), 1, f1))) {
	    free(tag_head);
	    free(head);
	    return(-12);
	}
	alttagdic[alttag->id] = (char *) malloc(BDF_FILE_MAX_ALTTAG_LENGTH+1);
	strcpy(alttagdic[alttag->id], alttag->alttag);
    }
    free(alttag);
    
    bh->head=head;
    bh->taghead=tag_head;
    bh->tagdic=tagdic;
    bh->alttaghead=alttag_head;
    bh->alttagdic=alttagdic;
    
    return(1);
}

/* 
 * bdf_readflow()
 *     
 *	read the next flow from the file and put the result in *flow
 *      It assumes the fileptr has advanced to the data block
 */
int bdf_readflow(FILE *f1, bdf_file_data *data)
{
    int n;

    if (! data)
	return(-1);
    if (!(n=fread(data, sizeof(bdf_file_data), 1, f1))) {
       return(0); 
    }

    return 1;
}
/* same as readflow , but attempts to read 'blocksize' number
 * of flows at once
 */
int bdf_readxflow(FILE *f1, bdf_file_block *block, int blocksize)
{
    int n;

    if (! block)
	return(-1);

    if (!(n=fread(block, sizeof(bdf_file_data), blocksize, f1))) {
       return(0); 
    }

    return n;
}

/* 
 * bdf_summary()
 *     Produces a summary of Bytes_from and Bytes_To by flowtag
 *	On error returns a < 0
 */
int bdf_summary(FILE *f1, struct summarydetails *sum_struct)
{
    int i, n, major, minor;

    bdf_file_header *head=NULL;
    bdf_file_tag_header *tag_head=NULL;
    bdf_file_tag *bdf_tag=NULL;
    bdf_file_alttag_header *alttag_head=NULL;
    bdf_file_alttag *alttag=NULL;
    bdf_file_data data;
    bdf_file_data_v5 datav5;

    for (i=0; i<=MAX_TAGS; i++) {
        sum_struct[i].total_to_bytes=0;
        sum_struct[i].total_from_bytes=0;
        sum_struct[i].count=0;
    }

    /* Seek to start */
    fseek(f1, 0,SEEK_SET);

    /* Read header */
    if (! ( head = (bdf_file_header *) malloc(sizeof(bdf_file_header))) ) {
	return(0);
    }
    if (! (n=fread(head, sizeof(bdf_file_header), 1, f1))) {
	free(head);
        return(-1);
    }

    major=(head->version>>16) & 0xFFFF;
    minor=head->version & 0xFFFF;

    if (major == 5 && minor == 0)
	VERSION=5;
    else if (major !=4 || (minor!=3 && minor!=4))  {
	free(head);
	return(-2);
    }

    /* Read the tags dictionary */      
    if (! ( tag_head = (bdf_file_tag_header *) malloc(sizeof(bdf_file_tag_header))) ) {
	free(head);
	return(-3);
    }
    if (! (n=fread(tag_head, sizeof(bdf_file_tag_header), 1, f1))) {
	free(tag_head);
	free(head);
        return(-4);
    }

    /* Not used, move file ptr */
    if (! ( bdf_tag = (bdf_file_tag *) malloc(sizeof(bdf_file_tag))) ) {
	    free(tag_head);
	    free(head);
	    return(-5);
    }
    for (i=0; i< tag_head->rows;i++) {
        if (! (n=fread(bdf_tag, sizeof(bdf_file_tag), 1, f1))) {
	    free(tag_head);
	    free(head);
	    return(-12);
	}
    }
    free(bdf_tag);

    if (! ( alttag_head = (bdf_file_alttag_header *) malloc(sizeof(bdf_file_alttag_header))) ) {
	free(tag_head);
	free(head);
	return(-6);
    }
    if (! (n=fread(alttag_head, sizeof(bdf_file_alttag_header), 1, f1))) {
	free(tag_head);
	free(head);
	return(-6);
    }

    /* For number of alttags, read */
    if (! ( alttag = (bdf_file_alttag *) malloc(sizeof(bdf_file_alttag))) ) {
	    free(tag_head);
	    free(head);
	    return(-7);
    }
    for (i=0; i< alttag_head->rows;i++) {
	if (! (n=fread(alttag, sizeof(bdf_file_alttag), 1, f1))) {
	    free(alttag);
	    free(tag_head);
	    free(head);
	    return(-10);
	}
	/* Not implemented */
    }

    if (VERSION==4) {
        while  ( (n=fread(&data, sizeof(data), 1, f1)) ) {
	    sum_struct[data.tag_id].total_to_bytes = sum_struct[data.tag_id].total_to_bytes + data.bytes_to;
	    sum_struct[data.tag_id].total_from_bytes = sum_struct[data.tag_id].total_from_bytes + data.bytes_from;
	    sum_struct[data.tag_id].count = sum_struct[data.tag_id].count + 1;
        }

    } else if (VERSION==5) {
        while  ( (n=fread(&datav5, sizeof(datav5), 1, f1)) ) {
	    sum_struct[datav5.tag_id].total_to_bytes = sum_struct[datav5.tag_id].total_to_bytes + datav5.bytes_to;
	    sum_struct[datav5.tag_id].total_from_bytes = sum_struct[datav5.tag_id].total_from_bytes + datav5.bytes_from;
	    sum_struct[datav5.tag_id].count = sum_struct[datav5.tag_id].count + 1;
        }

    }

    free(alttag);
    free(tag_head);
    free(head);
    free(alttag_head);

    return(1);
}

char *bdf_errlookup(int err)
{
    return(error_str[abs(err)]);
}

/***
 * ip int to ipstr
 ***/
char *ip2str(unsigned long ip, char *str)
{

    register int b1, b2, b3, b4;

    b1 = (ip & 0xFF000000L) >> 24;
    b2 = (ip & 0x00FF0000L) >> 16;
    b3 = (ip & 0x0000FF00L) >> 8;
    b4 = ip & 0x000000FFL;

    sprintf(str, "%d.%d.%d.%d",b1, b2, b3, b4);

    return (str);
}

/*
 * free_bdf_headers()
 *	Free all data to do with the bdf_headers structure
 *
 */
int free_bdf_headers(struct bdf_headers *bh)
{

    if (!bh) return -1;

    if (bh->head) free(bh->head); 
    if (bh->taghead) free(bh->taghead);
    if (bh->alttaghead) free(bh->alttaghead);
    if (bh->tagdic)  free_tagdic(bh->tagdic); 
    if (bh->alttagdic) free_alttagdic(bh->alttagdic);
    if (bh) free(bh);

    return 1;
}

/**
 * Clean up the flowtags in the tagdic ptr array
 */
int free_tagdic(char **tagdic) 
{

    int i;
   
    if (tagdic) { 
        for (i=0;i<MAX_TAGS; i++) {
            if (tagdic[i])
	        free(tagdic[i]);
	    else
	       break;
        }

        free(tagdic);
    }
    return 1;
}

int free_alttagdic(char **alttagdic) 
{
    int i;
   
    if (alttagdic) {
        for (i=0;i<MAX_ALTTAGS; i++) {
            if (alttagdic[i])
	        free(alttagdic[i]);
	    else
	       break;
        }

        free(alttagdic);
    }
    return 1;
}

