#include <thread>
#include <time.h>
#include <stdio.h> 
#include <iostream>
#include <string.h> 	// memset, memcpy 
#include <sys/socket.h> // socket API 
#include <netinet/in.h> // sockaddr_in
#include <unistd.h> 	// close()
#include <netdb.h> 		// gethostbyname
#include <stdlib.h> 	// atoi, rand 

#include "server.h"

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

    OutputBuffer oBuffer;
    srand(time(NULL));
    oBuffer.setInitSeq(rand() % MAXSEQNO);

    cerr << "Waiting for SYN" << endl;
    int ret;
    while(true) {
        // wait for SYN
        memset(buf,'\0', BUFSIZE);
        ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen);
        if (ret < 0)
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
            break;
        }
        else {
            perror("SYN packet didn't have SYN set");
            continue; 
        }
    }

    // set up connection buffers & variables here 
    cerr << "About to respond with SYN/ACK" << endl;
    // respond with SYNACK (seq. no = random, ack. no = client's seq. no + 1)
    Packet synack;
    synack.setSYN(); 
    synack.setAckNo(ack_to_client);
    synack.setACK(); 

    while(true) {
        if(oBuffer.hasSpace()) {
            uint16_t s1 = oBuffer.insert(synack);
            if (sendto(sockfd, oBuffer.getSeg(s1).data(), oBuffer.getSeg(s1).size(), 0 , (struct sockaddr *) &clientaddr, addrlen) < 0) {
                perror("sendto(): SYNACK"); 
            }
            memset(buf,'\0', BUFSIZE);
            cerr << "Sent SYNACK" << endl;
            ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen);
            if (ret < 0)
            {
                perror("recvfrom(): start of connection"); 
            }
            cerr << "Received ACK" << endl;
            Segment s (buf, buf + ret);
            Packet pkt(s); 
            pkt.toString();
            if (pkt.getSeqNo() == (ack_to_client) && pkt.hasACK() && pkt.getAckNo() == s1) {
                oBuffer.ack(pkt.getAckNo());
                break; 
            }
            else {
                cerr << "Invalid ACK" << endl;
            }
        }
        else {
            cerr << "No space" << endl;
        }
    }
    
        // else continue    
    cerr << "Handshake complete" << endl;


    FileReader reader(argv[2]);
    while (reader.hasMore()) {
        Packet response;
        response.setData(reader.top());
        reader.pop(); 

        while(!oBuffer.hasSpace(response.getData().size())) {
            //if buffer has no space, wait for ack to clear up space
            ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen);
            cerr << "Received ACK to data packet" << endl;
            Segment s (buf, buf + ret);
            Packet pkt(s); 
            pkt.toString();
            if (pkt.hasACK()) {
                oBuffer.ack(pkt.getAckNo());
                break; 
            }
            else {
                cerr << "Invalid ACK" << endl;
            }
        }

        uint16_t dataSeqNo = oBuffer.insert(response);
        cerr << "Sending file chunk" << endl;
        response.toString();
        if (sendto(sockfd, oBuffer.getSeg(dataSeqNo).data(), oBuffer.getSeg(dataSeqNo).size(), 0 , (struct sockaddr *) &clientaddr, addrlen) < 0)
        {
            perror("sendto(): FILE"); 
        }
    }

    
    while (!oBuffer.isEmpty()) {
        cerr << "BUFFER NOT EMPTY: " << endl;
        oBuffer.toString();
        ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen);
        cerr << "Received ACK to data packet" << endl;
        Segment s (buf, buf + ret);
        Packet pkt(s); 
        pkt.toString();
        if (pkt.hasACK()) {
            oBuffer.ack(pkt.getAckNo());
        }
        else {
            cerr << "Invalid ACK" << endl;
        }
    }
    
    cerr << "Entering teardown" << endl;
    oBuffer.toString();


    // int fd = open(argv[2], O_RDONLY);
    // memset(buf,'\0', BUFSIZE);
    // while ( (ret = read(fd, buf, BUFSIZE)) > 0) {
    //     cerr << "ret: " << ret << endl;
    //     Packet response;
    //     Data payload(buf, buf + ret);
    //     response.setData(payload);
    //     // seed later 
    //     response.setSeqNo(serv_seqno);
    //     Segment file = response.encode();
    //     cerr << "Sending file chunk" << endl;
    //     response.toString();
    //     if (sendto(sockfd, file.data(), file.size(), 0 , (struct sockaddr *) &clientaddr, addrlen) < 0)
    //     {
    //         perror("sendto(): FILE"); 
    //     }
    //     serv_seqno += ret;
    //     memset(buf,'\0', BUFSIZE);
    //close(fd);
    // TODO: Start file transfer
    // 1) Separate into segments of max size 1024 bytes (8192 bits) 
    // 2) Start sending 
    // 3) Keep in mind congestion window size & unacknowledged packets 
    // 4) Resend packets if needed 

    //TCP teardown
    //send FIN
    Packet fin;
    fin.setFIN();
    uint16_t finAckNo = oBuffer.insert(fin);
    while(true) {
        if (sendto(sockfd, oBuffer.getSeg(finAckNo).data(), oBuffer.getSeg(finAckNo).size(), 0 , (struct sockaddr *) &clientaddr, addrlen) < 0) {
            perror("sendto(): FIN"); 
        }
        memset(buf,'\0', BUFSIZE);
        ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen);
        if (ret < 0) {
            perror("recvfrom(): teardown ACK"); 
            memset(buf,'\0', BUFSIZE);
            continue;
        }
        cerr << "Server received packet (teardown ACK)" << endl;
        Segment s(buf, buf + ret);
        Packet pkt(s);
        pkt.toString();
        if(pkt.hasACK() && pkt.getAckNo() == finAckNo) {
            oBuffer.ack(finAckNo);
            //wait for FIN
            while(true) {
                memset(buf,'\0', BUFSIZE);
                ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen);
                if (ret < 0) {
                    perror("recvfrom(): final FIN"); 
                }
                cerr << "Server received packet (teardown FIN)" << endl;
                Segment segF(buf, buf + ret);
                Packet pF(segF);
                pF.toString();
                if(pF.hasFIN()) {
                    Packet finalAck;
                    finalAck.setAckNo(ack_to_client);
                    finalAck.setACK();
                    uint16_t finalAckSeqNo = oBuffer.insert(finalAck);
                    if (sendto(sockfd, oBuffer.getSeg(finalAckSeqNo).data(), oBuffer.getSeg(finalAckSeqNo).size(), 0 , (struct sockaddr *) &clientaddr, addrlen) < 0) {
                        perror("sendto(): final ACK"); 
                    }
                    close(sockfd);
                    return 0;
                }
                else {
                    perror("Invalid FIN");
                    memset(buf,'\0', BUFSIZE);
                    continue;
                }
                
            }
        }
        else {
            perror("Invalid teardown ACK");
            memset(buf,'\0', BUFSIZE);
            continue;
        }
    }
    return 0;
}