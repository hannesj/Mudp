#include "mudpc.h"


/* Internal data structures for association management */

/*unsigned short aid_start = 0X01;*/

unsigned short port_start = MUDP_DATA_PORT_START;

int local_socks [MAX_BIND_ADDRS];

unsigned int local_addrs[MAX_BIND_ADDRS];

unsigned char num_socks;

CLIENT_ASSOC client_socks [MAX_CLIENT_CONNECTIONS];

MUDPC_EVENT_CALLBACK server_cb;

pthread_t client_threads [MAX_CLIENT_CONNECTIONS];

void * read_thread (void * arg);
void * conn_thread (void * arg);

int sockfd;

mudpc_socket_t mudp_socket_create(
                   unsigned char num_addrs,
                   char  (*bind_addrs)[20],
                   unsigned short port,
                   MUDPC_EVENT_CALLBACK cb
               )
{
    unsigned char i;
    struct sockaddr_in addr;
    pthread_t thread_id;

    /* Register the call back */
    if (NULL == cb)
    {
        return 0;
    }

    server_cb = cb;

    /* Copy the bind addresses */
    for (i = 0; (i < num_addrs && i < MAX_BIND_ADDRS); i++)
    {
        local_addrs[i] = inet_addr (bind_addrs[i]);
    }

    num_socks = i;

    /* Bind on the first address to accept incomming connections */
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        return 0;
    }
    {
        printf ("Socket Created - %d\r\n", sockfd);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr (bind_addrs[0]);
    memset(addr.sin_zero, 0x0, sizeof (addr.sin_zero));

    if (bind (sockfd,(struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
        return 0;
    }

    /* Start a thread for accepting connection from clients */
    if (pthread_create(&thread_id, NULL, conn_thread, (void*) &sockfd) != 0)
    {
        printf ("Thread creation failed\r\n");
        return 0;
    }
    
    /* <TODO> Currently single instance allowed*/
    return 1;
}


void * conn_thread (void * arg)
{
    int sock = *((int *)arg);
    unsigned char buf[100],rsp_buf[100];
    unsigned char gid;
    int recv_len, i, offset;
    struct sockaddr_in client;
    unsigned short data_port;
    pthread_t thread_id;
    socklen_t len=sizeof(struct sockaddr);
    /* Wait for connections */
    printf ("Socket Created - %d\r\n", sock);

    while (1)
    {

        /* Read on the socket */
        if ((recv_len = recvfrom(sock, buf, 100, 0, (struct sockaddr *)&client, &len)) < 0)
        {
            perror ("recvfrom");
            printf ("Sock read failed\r\n");
            return NULL;
        }

        /* Check if it a connection request */
        if (buf[0] != MUDP_ASSO_REQ)
        {
            /* Spurious packet received */
            continue;
        }
        
        gid = buf[1];
        printf("G_ID: %d\r\n",gid);
        printf("thread_id: %lu\r\n",client_threads[gid]);
        
        if (client_threads[gid] != 0){
            for(i = 0; i < MAX_BIND_ADDRS; i++)
            {
                if (client_socks[gid].sd[i] != 0)
                {
                    shutdown(client_socks[gid].sd[i],SHUT_RDWR);
                    client_socks[gid].sd[i] = 0;
                }
            }
            client_threads[gid] = 0;
        
        }

        /* Create a client instance for the connection received */
        client_socks[gid].aid = gid;
        /* To be set by the user once the notification is sent */
        client_socks[gid].cb = NULL;
        client_socks[gid].client = client;
        client_socks[gid].local_sock = 0;
        client_socks[gid].port = port_start;

        /* Notifiy the application */
        server_cb (gid, 0x01, &buf[3], recv_len-3);

        /* Bind server sockets for client association */
        for (i = 0; i < num_socks; i++)
        {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port_start);
            addr.sin_addr.s_addr = local_addrs[i];
            memset(addr.sin_zero, 0x0, sizeof (addr.sin_zero));

            client_socks[gid].sd[i] = socket(PF_INET, SOCK_DGRAM, 0);
            if (client_socks[gid].sd[i] < 0)
            {
                perror ("socket");
                return NULL;
            }

            if (bind (client_socks[gid].sd[i],(struct sockaddr *)&addr, sizeof (addr)) < 0)
            {
                perror ("bind");
                return NULL;
            }
        }
        
        printf ("Binding to all server sockets completed\r\n");

        client_socks[gid].local_sock = client_socks[gid].sd[0];

        /* Create a thread for the client connection reads*/
        if (pthread_create(&thread_id, NULL, read_thread, (void*) &client_socks[gid].aid) != 0)
        {
            printf ("Thread creation failed\r\n");
            return NULL;
        }
        printf ("thread: %lu\n",thread_id); 
        client_threads[gid] = thread_id;

        /* Send association response */
        rsp_buf[0] = MUDP_ASSO_RSP;
        rsp_buf[1] = (unsigned char)(gid >> 8);
        rsp_buf[2] = (unsigned char)(gid & 0xFF);
        rsp_buf[3] = num_socks;
        data_port = htons (port_start);
        memcpy((rsp_buf+4), &data_port, 0x02);

        offset = 6;

        for (i = 0; i < num_socks; i++)
        {
            memcpy((rsp_buf+offset), &(local_addrs[i]), sizeof(unsigned int));
            offset +=4;
        }
        
        printf ("Sending %d\r\n", len);
        printf ("Port %d\r\n", client.sin_port);
        printf ("Port %d\r\n", client.sin_addr.s_addr);
        memset(client.sin_zero, 0x0, sizeof (client.sin_zero));

        if (sendto(sock, rsp_buf, offset, 0, (struct sockaddr *)&client, sizeof(struct sockaddr))<0)
        {
            perror ("sendto");
            return NULL;
        }

        /* Update the port */
        port_start++;
    }
}

void mudp_register_data_cb (mudpc_socket_t aid, MUDPC_DATA_CALLBACK cb)
{
    if (aid > MAX_CLIENT_CONNECTIONS || cb == NULL)
    {
        return;
    }

    client_socks[aid].cb = cb;

    return;
}

void * read_thread (void *arg)
{
    unsigned short aid = *((unsigned short *)arg);
    fd_set read_set, read_set_u;
    int i, s, max_fd, read;
    struct sockaddr_in addr;
    unsigned char buf[MAX_BUFLEN];
    socklen_t len = sizeof (addr);
    struct timeval timeout, timeout_master;
    
    /* Set a 90 sec timeout */
    timeout_master.tv_sec = 90;
    timeout_master.tv_usec = 0;
       
    printf ("Inside read thread : %d\r\n", aid);

    /* Clear the FD set */
    FD_ZERO (&read_set);

    /* Set the FDs for monitoring */
    max_fd = 0;
    for (i = 0; i < num_socks; i++)
    {
        FD_SET(client_socks[aid].sd[i], &read_set_u);
        if (max_fd < client_socks[aid].sd[i])
        {
            max_fd = client_socks[aid].sd[i];
        }
    }

    while (1)
    {
        read_set = read_set_u;        
        timeout = timeout_master;
        
        /* Start a blocking select on all the sockets */
        if(select(max_fd+1, &read_set, NULL, NULL, NULL) <= 0)
        /* if(select(max_fd+1, &read_set, NULL, NULL, &timeout) == -1) */
        {
            perror("select");
            return NULL;
        }

        for (i = 0; i < num_socks; i++)
        {
            if(FD_ISSET(client_socks[aid].sd[i], &read_set))
            {
                s = client_socks[aid].sd[i];
                //s++;
                //break;
            }
        }

        /* Read data from the socket */
        if ((read = recvfrom(s, buf, MAX_BUFLEN, 0, (struct sockaddr*)&addr, &len)) <= 0)
        {
            perror ("recvfrom");
            return NULL;
        }

        /* Check the kind of request */

        switch (buf[0])
        {

        case MUDP_HEARTBEAT_PRI:
            /* Set local and remote association socket */
            client_socks[aid].client = addr;
            client_socks[aid].local_sock = s;
            printf ("PRIMARY LINK CHANGED\r\n");
        case MUDP_HEARTBEAT_REQ:
            //printf ("H-%d\r\n", buf[3]);
            /* Send the response back */
            buf [0] = MUDP_HEARTBEAT_RSP;
            if (sendto(s, buf, read, 0, (struct sockaddr *)&addr, sizeof (struct sockaddr)) <0 )
            {
                perror ("sendto heartbeat");
            }
            break;
        case MUDP_DATA:

            /* invoke the application data callback */
            client_socks[aid].cb(aid, buf+3, read-3);
            break;
        }
    }
}

int mudpc_send (unsigned short aid, void * buf, unsigned int buf_len)
{
    unsigned char send_buffer [MAX_BUFLEN];
    int n;

    send_buffer [0] = MUDP_DATA;
    send_buffer [1] = (unsigned char)(aid >> 8);
    send_buffer [2] = (unsigned char)(aid & 0xFF);

    memcpy (&send_buffer[3], buf, buf_len);

    /* Send the buffer across */
    if ((n=sendto(client_socks[aid].local_sock, send_buffer, 0x03+buf_len, 0,
            (struct sockaddr *)&client_socks[aid].client, sizeof(struct sockaddr))) < 0)
    {
        perror ("sendto data");
    }

    return n;
}

