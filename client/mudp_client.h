/*
 * mudp_client.h
 *
 *  Created on: Jul 31, 2011
 *      Author: vinay
 */

#ifndef MUDP_CLIENT_H_
#define MUDP_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Tunable parameter definition */

/* Maximum number of local addresses to bind */
#define MAX_BIND_ADDRS 3

/* Value in seconds to send out link heart beat */
#define HEART_BEAT_INTERVAL 1

/* Type definition for the MUDP client socket type */
typedef int mudpc_socket_t;

/* MUDP event call back */

typedef void (* MUDPC_EVENT_CALLBACK)
             (
                 unsigned char type,
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
        in_addr_t addr_local, 
        char * addr,
        unsigned short port
    );

int mudpc_send (mudpc_socket_t sock, void * buf, unsigned int buf_len);

int mudpc_disconnect (mudpc_socket_t sock);

#endif /* MUDP_CLIENT_H_ */
