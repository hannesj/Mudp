
#define _BSD_SOURCE 1

#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <string.h>
#include <pcap.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include "mudpc.h"

#define ETHERNET_HEADER_LEN 14
#define BUFFER_SIZE (1 << 16)
#define PORT 4135
#define MSG_SIZE 1000
#define NUMBER_OF_MESSAGES 10000

int flags, s_fd;
mudpc_socket_t sock;
struct sockaddr_in *paddrs[5];
struct sockaddr_in *laddrs[5];
unsigned char recv_buffer[BUFFER_SIZE];
void handle_signal(int signum);
void read_cb (unsigned char aid, void * buf, int buf_len);
void process_pkt(u_char *args, const struct pcap_pkthdr *header,
        const u_char *p);

unsigned long client_ip;

unsigned char pkt[2000];

int main(int argc, char **argv)
{
    int i;
    int counter = 0;
    int asconf = 1;
    int ret;
    int addr_count;
    char buffer[MSG_SIZE];
    sctp_assoc_t id;
    struct sockaddr_in addr, temp;
    char local_addr[4][20];

    pthread_t thread_id;

    pcap_t * handle;
    char error_buf[PCAP_ERRBUF_SIZE];
    int optval;

    if(argc < 4 )
    {
        puts("Usage: ERROR");        
        return 0;
    }

    /* Try to open the network interface for sniffing */
    handle = pcap_open_live(argv[1], 2000, 1, 1000, error_buf);
    if (NULL == handle)
    {
        printf ("Error opening the device for sniffing %s \r\n", error_buf);
        return -1;
    }
    
    /* Open the raw socket for sending the constucted ip packets */
    s_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    if (s_fd < 0)
    {
        printf ("Error in creating RAW socket \r\n");
    }
    
    /* Set socket options */
    optval = 1;
    setsockopt(s_fd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int));
    perror("setsockopt");


    for (i = 0; i < (argc-4); i++)
    {
        /* Read the local address to bind to */
        strcpy(local_addr[i], argv[2+i]);
    }

    inet_aton(argv[2+i], &(temp.sin_addr));
    client_ip = temp.sin_addr.s_addr;

    /* Create a MUDP socket */
    sock = mudp_socket_create(
                   i,
                   local_addr,
                   9900,
                   read_cb
               );

    if (sock != 1)
    {
        printf ("Socket Creation failed\r\n");
        exit (0);
    }
    
    /* Create a connection with the server */
    if (mudpc_connect (sock, argv[3+i], 9900) < 0)
    {
        printf ("Connection failed");
    }

    /* Start to capture the packets */
    pcap_loop(handle, -1, process_pkt, NULL);



}

void read_cb (unsigned char aid, void * buf, int buf_len)
{
    struct sockaddr_in addr;
    struct sockaddr_in dest;
    int ret;
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = client_ip;

    /* Send the packet over the raw socket */
    if ((ret = sendto(s_fd, buf, buf_len, 0, (struct sockaddr *)&dest, sizeof(struct sockaddr))) < 0)
    {
        printf ("Sending over raw socket failed!!\r\n");
        perror("sendto");
    }
    else
    {
        printf("%x -> RAW \r\n", ret);
    }
    return;
}

void process_pkt(u_char *args, const struct pcap_pkthdr *header,
        const u_char *p)
{
    struct iphdr *ip_hdr;
    //struct udphdr *hdr;
    int ret;
    memset (pkt, 0x00, 2000);
    
   ip_hdr = (struct iphdr *)(p);
    
    if ((ip_hdr->ihl < 5) && (ip_hdr->ihl > 15))
    {
        printf("CRAP\r\n");
        return;
    }
   // printf ("%x-->%x-->%x\r\n", (ip_hdr->ihl), (ip_hdr->version), ip_hdr->protocol);
    
    if (ip_hdr->saddr != client_ip)
    {
             return;
    }
    /* Copy the packet in to a buffer */
    memcpy (pkt, (p), header->caplen);

    /* Send the pack over MUDP */
    mudpc_send(sock, pkt, header->caplen);

    //printf("%d <- PCAP\n",header->caplen-ETHERNET_HEADER_LEN);

    return;
}

