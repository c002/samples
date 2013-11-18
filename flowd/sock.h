#ifndef SOCK_H
/*
 * sock.h
 *
 * Definitions of API to the netflow listener for the
 * BTA V4 Netflow collector
 *
 * Author: Mike McCauley (mikem@open.com.au)
 * Copyright (C) 1998 connect.com.au pty ltd
 * $Id: sock.h,v 1.1.1.1 2004/05/29 09:06:42 harry Exp $
 */
#define SOCK_H

/* Required by flowdata.h */
typedef ulong ipaddrtype;
typedef unsigned char uchar;

#define FLOWSTATHDR_VERSION_1 1
#define FLOWSTATHDR_VERSION_5 5
#define FLOWSTATHDR_VERSION_6 6
 
/*
 * We need a big recv buffer, so activity in other processes 
 * wont mean dropped netflow packets. Administrator must allow 
 * them this big with something like:
 * % /usr/sbin/ndd -set /dev/udp udp_max_buf 5000000
 */
#define SOCK_SOCKET_BUFFER_SIZE 5000000
/* #define SOCK_SOCKET_BUFFER_SIZE 10000 */
#define SOCK_SOCKET_BUFFER_SIZE_FALLBACK 200000

/*
 * Return some text explaining why a function call failed
 */
extern const char* sock_last_errmsg();

extern int sock_init(int port);
extern int sock_handle_one_packet();

/* 
 * This structure allows us to keep info about each router we have seen.
 * We are most interested in the sequence number, since we need to track 
 * sequence numbers by the originating router
 */
typedef struct 
{
    ulong_t	routip;    /* IP address of the router */
    ulong_t	next_seq;  /* The expected sequence number of the next packet */ 
    uchar	engine_type; /* Type of flow-switching engine */
    uchar	engine_id; /* The engine ID of the Netflow engine */
    ulong_t	sysuptime; /* Time in milliseconds since last reboot */
}   router;

#endif
