#include <string>
#include <thread>
#include <stdio.h> 
#include <iostream>
#include <string.h> 	// memset, memcpy 
#include <sys/socket.h> // socket API 
#include <netinet/in.h> // sockaddr_in
#include <unistd.h> 	// close()
#include <netdb.h> 		// gethostbyname
#include <stdlib.h> 	// atoi, rand 
#include <fcntl.h>

#include "server.h"

#define BUFSIZE 1032    // buffer size 
#define PORT 4000       // default port 
#define MAX_SEQ_NO 30720 // in bytes 
using namespace std;

int main(int argc, char **argv) {
    // Parse command-line args 
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <PORT-NUMBER>" << "<FILENAME>" << endl; 
        exit(1); 
    }

    int port = atoi(argv[1]); 
    
	struct sockaddr_in myaddr;      /* our address */
    struct sockaddr_in clientaddr;     /* remote address */
    socklen_t addrlen = sizeof(clientaddr);            /* length of addresses */
    //int recvlen;                    /* # bytes received */
    int sockfd;                         /* our socket */
    uint8_t buf[BUFSIZE];     /* receive buffer */

    /* create a UDP socket */

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        exit(1); 
    }

    /* bind the socket to any valid IP address and a specific port */

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(port);
    //need the :: because otherwise it uses the wrong function
    if (::bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) { 
        perror("bind failed");
        exit(1); 
    }

    // DONE: Set up TCP connection 
    // 1) wait for SYN 
    // 2) respond with SYN (seq. no = random, ack. no = client's seq. no + 1)
    // 3) wait for SYN = 0 (ack. no = our own seq. no + 1, seq. no = client's + 1)

    uint16_t ack_to_client; 
    uint16_t serv_seqno; 
    cerr << "Waiting for SYN" << endl;
    while(true) {
        // wait for SYN
        memset(buf,'\0', BUFSIZE);
        if (recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen) == -1)
        {
            perror("recvfrom(): SYN"); 
        }
        cerr << "Server received packet (SYN)" << endl;
        Segment s(buf, buf + sizeof(buf)/sizeof(buf[0]));
        Packet pkt(s);
        pkt.toString();
        //if valid SYN, break and respond to client 
        if (pkt.hasSYN()) {
            ack_to_client = pkt.getSeqNo() + 1; 
            serv_seqno = rand() % MAX_SEQ_NO; 
            break;
        }
        else {
            cerr << "SYN packet didn't have SYN set" << endl;
            continue; 
        }
    }

    // set up connection buffers & variables here 
    cerr << "About to respond with SYN/ACK" << endl;
    while(true) {
        // respond with SYN (seq. no = random, ack. no = client's seq. no + 1)
        Packet synack; 
        synack.setSYN(); 
        synack.setACK(); 
        synack.setSeqNo(serv_seqno); 
        synack.setAckNo(ack_to_client); 
        Segment sav = synack.encode(); 
        if (sendto(sockfd, sav.data(), sizeof(sav), 0 , (struct sockaddr *) &clientaddr, addrlen) == -1)
        {
            perror("sendto(): SYNACK"); 
        }
        memset(buf,'\0', BUFSIZE);
        cerr << "Sent SYNACK" << endl;
        if (recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen) == -1)
        {
            perror("recvfrom(): start of connection"); 
        }
        cerr << "Received ACK" << endl;
        Segment s (buf, buf + sizeof(buf)/sizeof(buf[0]));
        Packet pkt(s); 
        pkt.toString();
        if (pkt.getSeqNo() == (ack_to_client) && pkt.hasACK() && pkt.getAckNo() == (serv_seqno+1)){
            // increment sequence number & ack number doesn't matter? 
            serv_seqno++;
            break; 
        }
        // else continue 
    }
    cerr << "Handshake complete" << endl;

    int fd = open(argv[2], O_RDONLY);
    int ret;
    while ( (ret = read(fd, buf, sizeof(buf))) != 0) {
        Packet response;
        Data payload(buf, buf + ret);
        response.setData(payload);
        response.setSeqNo(serv_seqno);
        Segment file = response.encode();
        cerr << "Sending file chunk" << endl;
        response.toString();
        if (sendto(sockfd, file.data(), sizeof(file), 0 , (struct sockaddr *) &clientaddr, addrlen) == -1)
        {
            perror("sendto(): FILE"); 
        }
        serv_seqno += ret;
        memset(buf,'\0', BUFSIZE);
    }
    close(fd);
    // TODO: Start file transfer
    // 1) Separate into segments of max size 1024 bytes (8192 bits) 
    // 2) Start sending 
    // 3) Keep in mind congestion window size & unacknowledged packets 
    // 4) Resend packets if needed 

    //TCP teardown
    while(true) {
        // wait for FIN
        memset(buf,'\0', BUFSIZE);
        if (recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen) == -1)
        {
            perror("recvfrom(): FIN"); 
        }
        cerr << "Server received packet (FIN)" << endl;
        Segment s(buf, buf + sizeof(buf)/sizeof(buf[0]));
        Packet pkt(s);
        pkt.toString();
        if (pkt.getSeqNo() == ack_to_client) {
            if(pkt.hasFIN()) {
                //if valid FIN, proceed to send ACK
                ack_to_client = pkt.getSeqNo() + 1;
                Packet ack;
                ack.setSeqNo(serv_seqno);
                ack.setAckNo(ack_to_client);
                ack.setACK(); 
                Segment a = ack.encode();
                if (sendto(sockfd, a.data(), sizeof(a), 0 , (struct sockaddr *) &clientaddr, addrlen) == -1) {
                    perror("sendto(): ACK response to FIN"); 
                }

                //after sending ACK, send FIN
                Packet fin;
                fin.setSeqNo(serv_seqno);
                fin.setFIN();
                Segment f = fin.encode();
                if (sendto(sockfd, f.data(), sizeof(f), 0 , (struct sockaddr *) &clientaddr, addrlen) == -1) {
                    perror("sendto(): FIN response to FIN"); 
                }

                //wait for final ACK
                cerr << "Waiting for final ACK" << endl;
                while(true) {
                    memset(buf,'\0', BUFSIZE);
                    if (recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen) == -1) {
                        perror("recvfrom(): final ACK"); 
                    }
                    cerr << "Server received final ACK" << endl;
                    Segment final(buf, buf + sizeof(buf)/sizeof(buf[0]));
                    Packet finalPacket(final);
                    finalPacket.toString();
                    //upon receipt of ACK, close socket
                    if(finalPacket.hasACK() && finalPacket.getAckNo() == serv_seqno + 1) {
                        close(sockfd);
                        return 0;
                    }
                    else {
                        cerr << "Final ACK response: invalid ACK" << endl;
                        continue;
                    }
                }
            }
            else {
                cerr << "FIN packet: FIN not set" << endl;
                continue;
            }
        }
        else {
            cerr << "FIN packet: invalid seqNo" << endl;
            continue; 
        }
    }

    return 0;
}