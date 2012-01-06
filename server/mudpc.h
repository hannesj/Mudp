/*
 * mudpc.h
 *
 *  Created on: Jul 31, 2011
 *      Author: vinay
 */

#ifndef MUDPC_H_
#define MUDPC_H_

#include "mudp_server.h"

/* Protocol opcodes */
#define MUDP_ASSO_REQ      0x01
#define MUDP_ASSO_RSP      0x02
#define MUDP_DISASSO_REQ   0x07
#define MUDP_DISASSO_RSP   0x08
#define MUDP_HEARTBEAT_REQ 0x09
#define MUDP_HEARTBEAT_PRI 0x0B
#define MUDP_HEARTBEAT_RSP 0x0C
#define MUDP_DATA          0x0F

/* Protocol state definitions */
#define MUDP_STATE_IDLE           0x00
#define MUDP_STATE_INIT           0x01
#define MUDP_STATE_ASSOCIATING    0x02
#define MUDP_STATE_ASSOCIATED     0x03
#define MUDP_STATE_SWITCHING      0x04
#define MUDP_STATE_DISACCOCIATING 0x05

#define MAX_BUFLEN                2000


#define MUDP_DATA_PORT_START      40000

typedef struct _CLIENT_ASSOC_
{
	unsigned short aid;
	struct sockaddr_in client;
	int local_sock;
	MUDPC_DATA_CALLBACK cb;
	unsigned short port;
	int sd[MAX_BIND_ADDRS];

}CLIENT_ASSOC;


#endif /* MUDPC_H_ */
