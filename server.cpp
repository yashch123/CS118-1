#include <thread>
#include <stdio.h> 
#include <iostream>
#include <string.h> 	// memset, memcpy 
#include <sys/socket.h> // socket API 
#include <netinet/in.h> // sockaddr_in
#include <unistd.h> 	// close()
#include <netdb.h> 		// gethostbyname
#include <stdlib.h> 	// atoi, rand 

#include "server.h"

#define BUFSIZE 1032    // buffer size 
#define PORT 4000       // default port 

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
    if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) { 
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
    int ret;
    while(true) {
        // wait for SYN
        memset(buf,'\0', BUFSIZE);
        if ((ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen)) == -1)
        {
            perror("recvfrom(): SYN"); 
        }
        cerr << "Server received packet (SYN)" << endl;
        Segment s(buf, buf + ret);
        Packet pkt(s);
        pkt.toString();
        //if valid SYN, break and respond to client 
        if (pkt.hasSYN()) {
            ack_to_client = pkt.getSeqNo() + 1; 
            serv_seqno = rand() % MAXSEQNO; 
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
        if ((ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen)) == -1)
        {
            perror("recvfrom(): start of connection"); 
        }
        cerr << "Received ACK" << endl;
        Segment s (buf, buf + ret);
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
    memset(buf,'\0', BUFSIZE);
    while ( (ret = read(fd, buf, sizeof(buf))) != 0) {
        Packet response;
        Data payload(buf, buf + ret);
        response.setData(payload);
        // seed later 
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
        //send FIN
        Packet fin;
        fin.setSeqNo(serv_seqno);
        fin.setFIN();
        Segment f = fin.encode();
        if (sendto(sockfd, f.data(), sizeof(f), 0 , (struct sockaddr *) &clientaddr, addrlen) == -1) {
            perror("sendto(): FIN"); 
        }
        serv_seqno++;

        // wait for ACK
        while(true) {
            memset(buf,'\0', BUFSIZE);
            if ((ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen)) == -1)
            {
                perror("recvfrom(): teardown ACK"); 
                memset(buf,'\0', BUFSIZE);
                continue;
            }
            cerr << "Server received packet (teardown ACK)" << endl;
            Segment s(buf, buf + ret);
            Packet pkt(s);
            pkt.toString();
            if(pkt.hasACK() && pkt.getAckNo() == serv_seqno) {
                //wait for FIN
                while(true) {
                    memset(buf,'\0', BUFSIZE);
                    if ((ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen)) == -1) {
                        perror("recvfrom(): final ACK"); 
                    }
                    cerr << "Server received packet (teardown FIN)" << endl;
                    Segment segF(buf, buf + ret);
                    Packet pF(segF);
                    pF.toString();
                    if(pF.hasFIN()) {
                        Packet finalAck;
                        finalAck.setSeqNo(serv_seqno);
                        finalAck.setAckNo(ack_to_client);
                        finalAck.setACK();
                        Segment fA = finalAck.encode();
                        if (sendto(sockfd, fA.data(), sizeof(fA), 0 , (struct sockaddr *) &clientaddr, addrlen) == -1) {
                            perror("sendto(): final ACK"); 
                        }
                        close(sockfd);
                        return 0;
                    }
                    else {
                        cerr << "Invalid FIN" << endl;
                        memset(buf,'\0', BUFSIZE);
                        continue;
                    }
                }
            }
            else {
                cerr << "Invalid teardown ACK" << endl;
                memset(buf,'\0', BUFSIZE);
                continue;
            }
        }
    }
    return 0;
}