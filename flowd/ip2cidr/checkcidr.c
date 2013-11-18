/*
 * checkcidr.c
 * Utility program to check whether the cidr databse is OK
 *
 *
 * Author: Mike McCauley (mikem@open.com.au)
 * Copyright (C) 1998 connect.com.au pty ltd
 * $Id: checkcidr.c,v 1.1.1.1 2004/05/29 09:06:53 harry Exp $
 */
#include <sys/types.h>
#include <stdio.h>
#include "ip2cidr.h"

int
main(int argc, char** argv)
{
/*
    if (cidr_init("checkcidr") != 0)
*/
    if (cidr_init("bgp",0) != 0)
    {
	fprintf(stderr, "cidr_init failed. Exiting\n");
	exit(1);
    }
    else
    {
	if (cidr_watchdog_ok())
	{
	    printf("CIDR database is OK\n");
	    exit(0);
	}
	else
	{
	    fprintf(stderr, "CIDR database is stale\n");
	    exit(1);
	}
    }
}
