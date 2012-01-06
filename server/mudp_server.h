/*
 * mudp_client.h
 *
 *  Created on: Jul 31, 2011
 *      Author: vinay
 */

#ifndef MUDP_SERVER_H_
#define MUDP_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Tunable parameter definition */

/* Maximum number of local addresses to bind */
#define MAX_BIND_ADDRS 3

#define MAX_CLIENT_CONNECTIONS    0xFF

/* Type definition for the MUDP client socket type */
typedef int mudpc_socket_t;

/* MUDP event call back */
typedef void (* MUDPC_EVENT_CALLBACK)
             (
            	 unsigned short aid,
                 unsigned char type,
                 void * buf,
                 int buf_len
             );

/* MUDP data call back */
typedef void (* MUDPC_DATA_CALLBACK)
             (
                 unsigned short aid,
                 void * buf,
                 int buf_len
             );


/* MUDP Client APIs */

mudpc_socket_t mudp_socket_create
               (
                   unsigned char num_addrs,
                   char (*bind_addrs)[20],
                   unsigned short port,
                   MUDPC_EVENT_CALLBACK cb
               );

int mudpc_connect
    (
        mudpc_socket_t sock,
        char * addr,
        unsigned short port
    );

void mudp_register_data_cb (mudpc_socket_t aid, MUDPC_DATA_CALLBACK cb);

int mudpc_send (unsigned short aid, void * buf, unsigned int buf_len);

int mudpc_disconnect (mudpc_socket_t sock);

#endif /* MUDP_SERVER_H_ */
