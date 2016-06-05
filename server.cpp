#include <thread>
#include <time.h>
#include <stdio.h> 
#include <iostream>
#include <string.h>     // memset, memcpy 
#include <sys/socket.h> // socket API 
#include <netinet/in.h> // sockaddr_in
#include <unistd.h>     // close()
#include <netdb.h>      // gethostbyname
#include <stdlib.h>     // atoi, rand 

#include "server.h"

using namespace std;

enum state {
    SYNWAIT, 
    SYNRECEIVED,
    CONNECTED, 
    WAIT_FOR_ACKS,
    CLOSE,
    FINWAIT
};

int main(int argc, char **argv) {
    // Parse command-line args 
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <PORT-NUMBER> " << "<FILENAME>" << endl; 
        exit(1); 
    }

    // Set up socket and connect to desired port 
    int port = atoi(argv[1]); 
    
    struct sockaddr_in myaddr;      /* our address */
    struct sockaddr_in clientaddr;     /* remote address */
    socklen_t addrlen = sizeof(clientaddr);            /* length of addresses */
    //int recvlen;                    /* # bytes received */
    int sockfd;                         /* our socket */

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

    // Housekeeping 
    int ready_to_close = false; 
    state current_state = SYNWAIT; 

    // packet # variables, initialize to get rid of warnings
    uint16_t ack_no_to_client = 0;  
    uint16_t expected_ack_no = 0;
    uint16_t fin_ack_no = 0;

    // Buffers 
    uint8_t buf[BUFSIZE];     // receive packets from recvfrom 
    memset(buf,'\0', BUFSIZE);
    OutputBuffer oBuffer;     // store packets to be sent 
    FileReader reader(argv[2]);
    srand(time(NULL)); // seed 
    oBuffer.setInitSeq(rand() % MAXSEQNO);

    // cerr << "Waiting for SYN" << endl;
    int ret;  // Return value from recvfrom

    while(1) { 
        // Read from recvfrom 
        memset(buf,'\0', BUFSIZE);
        if ((ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen)) == -1){
            perror("recvfrom()"); 
        }
        // oBuffer.toString();
        Segment seg(buf, buf + ret);
        Packet current_packet(seg); // current packet being examined
        // current_packet.toString();

        switch(current_state) {
            case SYNWAIT: 
            {
                //cerr << "state SYNWAIT:" << endl; 
                // waiting to receive SYN, send SYN-ACK 
                // do timer later 

                // if the packet does not have SYN set, ignore it 
                if (!current_packet.hasSYN()) {
                    perror("SYN packet didn't have SYN set");
                    continue; 
                }

                ack_no_to_client = current_packet.getSeqNo() + 1; 
                current_state = SYNRECEIVED; 

                // send SYNACK 
                //cerr << "About to respond with SYN/ACK" << endl;
                // respond with SYNACK (seq. no = random, ack. no = client's seq. no + 1)
                Packet synack_packet;
                synack_packet.setSYN(); 
                synack_packet.setAckNo(ack_no_to_client);
                synack_packet.setACK(); 

                expected_ack_no = oBuffer.insert(synack_packet);
                if (sendto(sockfd, oBuffer.getSeg(expected_ack_no).data(), oBuffer.getSeg(expected_ack_no).size(), 0 , (struct sockaddr *) &clientaddr, addrlen) < 0) {
                    perror("sendto(): SYNACK"); 
                }
                //cerr << "Sent SYNACK" << endl;
                break;
            }
            case SYNRECEIVED:
            {
                // cerr << "state SYNRECEIVED:" << endl; 
                // need to implement timeout 
                if (current_packet.getSeqNo() == (ack_no_to_client) && current_packet.hasACK() && current_packet.getAckNo() == expected_ack_no) {
                    //cerr << "Received ACK" << endl;
                    oBuffer.ack(current_packet.getAckNo());
                    current_state = CONNECTED;
                    //cerr << "Handshake complete" << endl; 
                    // continue onto CONNECTED case 
                }
                else {
                    //cerr << "Invalid ACK" << endl;
                    continue;
                }
            }
            case CONNECTED: 
            {
                //cerr << "state CONNECTED:" << endl; 
                // send file chunks
                while(reader.hasMore()) {
                    Packet file_chunk;
                    file_chunk.setData(reader.top());
                    reader.pop(); 
                    while(!oBuffer.hasSpace(file_chunk.getData().size())) { 
                        //if buffer has no space, wait for ack to clear up space
                        ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &addrlen);
                        if (ret < 0)
                        {
                            perror("recvfrom(): Failed to Receive"); 
                        }
                        // oBuffer.toString();
                        //cerr << "Received ACK to data packet" << endl;
                        Segment s (buf, buf + ret);
                        Packet pkt(s); 
                        //pkt.toString();
                        if (pkt.hasACK()) {
                            oBuffer.ack(pkt.getAckNo());
                            break; 
                        }
                        else {
                            perror("Invalid ACK");
                        }
                    }

                    // output buffer not full (at least one spot open) so send another file chunk

                    uint16_t data_ack_no = oBuffer.insert(file_chunk);
                    //cerr << "Sending file chunk" << endl;
                    // file_chunk.toString();
                    if (sendto(sockfd, oBuffer.getSeg(data_ack_no).data(), oBuffer.getSeg(data_ack_no).size(), 0 , (struct sockaddr *) &clientaddr, addrlen) < 0)
                    {
                        perror("sendto(): FILE"); 
                    }
                }

                if(!reader.hasMore()) {
                    current_state = WAIT_FOR_ACKS; 
                }
                break;
            }
            case WAIT_FOR_ACKS: 
            {
                //cerr << "state WAIT_FOR_ACKS:" << endl; 
                // Wait for acks to all packets in output buffer 
                //cerr << "BUFFER NOT EMPTY: " << endl;
                // oBuffer.toString();
                if (current_packet.hasACK()) {
                    oBuffer.ack(current_packet.getAckNo());
                }
                else {
                    //cerr << "Invalid ACK" << endl;
                }

                // if empty move on to close state 
                if (oBuffer.isEmpty()) { 
                    current_state = CLOSE; 
                    //cerr << "Entering teardown" << endl; 
                    // send FIN
                    Packet fin_packet;
                    fin_packet.setFIN();
                    fin_ack_no = oBuffer.insert(fin_packet);
                    if (sendto(sockfd, oBuffer.getSeg(fin_ack_no).data(), oBuffer.getSeg(fin_ack_no).size(), 0 , (struct sockaddr *) &clientaddr, addrlen) < 0) {
                        perror("sendto(): FIN"); 
                    }
                }
                break; 
            }
            case CLOSE: 
            {
                //cerr << "state CLOSE:" << endl; 
                // close connection
                if(current_packet.hasACK() && current_packet.getAckNo() == fin_ack_no) {
                    oBuffer.ack(fin_ack_no);
                    current_state = FINWAIT; 
                }
                else {
                    perror("Invalid teardown ACK");
                    continue;
                }
                break;
            }
            case FINWAIT:
            {
                //cerr << "state FINWAIT:" << endl; 
                // wait for FIN
                if (current_packet.hasFIN()) { 
                    Packet final_ack_packet; 
                    final_ack_packet.setAckNo(ack_no_to_client);
                    final_ack_packet.setACK();
                    uint16_t final_ack_seq_no = oBuffer.insert(final_ack_packet);
                    if (sendto(sockfd, oBuffer.getSeg(final_ack_seq_no).data(), oBuffer.getSeg(final_ack_seq_no).size(), 0 , (struct sockaddr *) &clientaddr, addrlen) < 0) {
                        perror("sendto(): final ACK"); 
                    }
                    ready_to_close = true; 
                }
                else { 
                    perror("Invalid FIN");
                    continue;
                }
                break;
            }
        }
        if (ready_to_close)
            break; 
    }
    // wait for some time, then close 
    close(sockfd); 
    return 0; 
}