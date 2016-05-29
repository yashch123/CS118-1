#include <string>
#include <thread>
#include <stdio.h> 
#include <iostream>
#include <string.h> 	// memset, memcpy 
#include <sys/socket.h> // socket API 
#include <netinet/in.h> // sockaddr_in
#include <unistd.h> 	// close()
#include <netdb.h> 		// gethostbyname
#include <stdlib.h> 	// atoi 
#include "server.h"

#define BUFSIZE 8192
#define PORT 4000
using namespace std;

int main(int argc, char **argv) {
	////////////////////////////////
	// Don't trust this code ///////
	////////////////////////////////

	struct sockaddr_in myaddr;      /* our address */
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen = sizeof(remaddr);            /* length of addresses */
    int recvlen;                    /* # bytes received */
    int fd;                         /* our socket */
    unsigned char buf[BUFSIZE];     /* receive buffer */

    /* create a UDP socket */

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        exit(1); 
    }

    /* bind the socket to any valid IP address and a specific port */

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(PORT);

    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        exit(1); 
    }

    // TODO: Set up TCP connection 
    // 1) wait for SYN 
    // 2) respond with SYN (seq. no = random, ack. no = client's seq. no + 1)
    // 3) wait for SYN = 0 (ack. no = our own seq. no + 1, seq. no = client's + 1)

    // TODO: Start file transfer
    // 1) Separate into segments of max size 1024 bytes (8192 bits) 
    // 2) Start sending 
    // 3) Keep in mind congestion window size & unacknowledged packets 
    // 4) Resend packets if needed 
    
    // TODO: Terminate connection 
    // 1) Wait for a FIN from client 
    // 2) ACK the FIN
    // 3) send client FIN
    // 4) wait for ACK 

    close(fd);
    return 0;
}