#include <string>
#include <thread>
#include <iostream>
#include <iterator>
#include <stdio.h> 
#include <string.h> 	// memset, memcpy 
#include <sys/socket.h> // socket API
#include <sys/select.h> // select
#include <netinet/in.h> // sockaddr_in
#include <unistd.h> 	// close()
#include <netdb.h> 		// gethostbyname
#include <stdlib.h> 	// atoi, rand
#include <ctype.h>		// isalpha
#include <fstream>		// ofstream
#include <time.h>

#include "client.h"

using namespace std;

// finite state machine variables 
enum state {
	SYNWAIT,
	CONNECTED, 
	CLOSE
};

int main(int argc, char **argv)
{
	// Parse command line arguments 
	if (argc != 3) {
		cerr << "Usage: " << argv[0] << " <SERVER-HOST-OR-IP>" << " <PORT-NUMBER>" << endl; 
		exit(1); 
	}

	/*-----------------Setting up the Client-----------------*/
	// Decode hostname
	// argv[1] = hostname/IP, argv[2] = port #
	struct sockaddr_in servaddr;
	memset((char *)&servaddr, 0, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(atoi(argv[2])); 

	// Get host IP address 
	struct hostent *hp; 
	hp = gethostbyname(argv[1]); 
	if (hp)
		memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length); 
	else 
		memcpy((void *)&servaddr.sin_addr, argv[1], strlen(argv[1])); 

	// Create client socket
	int sockfd; 
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		perror("socket creation failed"); 
		exit(1); 
	}

	// bind to arbitrary return address since no application will initiate communication here
	// INADDR_ANY lets OS choose specific IP address 
	struct sockaddr_in myaddr; 
	memset((char *)&myaddr, 0, sizeof(myaddr)); 
	myaddr.sin_family = AF_INET; 
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	myaddr.sin_port = htons(0); // Let OS pick a port 

	if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) { 
		perror("bind failed"); 
		exit(1); 
	}

	// seed (for random sequence number) 
	srand(time(NULL));
	ReceivingBuffer rcvbuf;
	rcvbuf.setSeqNo(rand() % MAXSEQNO);


	/*-----------------Set up Timeout----------------------*/
	fd_set readFdswatchFds;
    FD_ZERO(&readFds);
    struct timeval tv;
    tv.usec = RTO;
    tv.sec = 0;
    int syn_tries = 0, fin_tries = 0;

	/*-----------------Begin TCP Handshake-----------------*/
	// Housekeeping 
	int ret;	// keep track of bytes read from recvfrom 
	uint8_t buf[BUFSIZE];
	memset(buf,'\0', BUFSIZE);
	socklen_t addrlen = sizeof(servaddr);
	uint16_t ackNo = 0;

	// Create syn 
	state current_state = SYNWAIT; 
	Packet syn_packet; 
	syn_packet.setSYN(); 
	syn_packet.setSeqNo(rcvbuf.getSeqNo());
	syn_packet.setRcvWin(CLIENTRCVWINDOW);
	Segment syn_encoded = syn_packet.encode(); 
	syn_packet.toString();
	if (sendto(sockfd, syn_encoded.data(), syn_encoded.size(), 0 , (struct sockaddr *) &servaddr, addrlen) < 0) {
	   	perror("sendto()"); 
	}
	FD_SET(sockfd, &readFds);

	/****************Enter finite state machine ***********/ 
	while(1) {
		bool retransmit = false;
		int nfds = 0; // start with no packets to be read
		FD_SET(sockfd, &watchFds);

		if (current_state == CLOSE) {
			// WAIT FOR SOMETIME
			rcvbuf.setSeqNo((rcvbuf.getSeqNo() + 1) % MAXSEQNO);
			Packet finPacket;
			finPacket.setFIN();
			finPacket.setSeqNo(rcvbuf.getSeqNo());
			finPacket.toString();
			Segment finSend = finPacket.encode();
			cout << "Sending FIN Packet";
			if (retransmit) {
				cout << " Retransmission";
			}
			cout << endl;
		}
			//finPacket.toString();
			if (sendto(sockfd, finSend.data(), finSend.size(), 0 , (struct sockaddr *) &servaddr, addrlen) < 0) {
	   			perror("sendto(): ACK"); 
	   		}
	   		break;
		}
		memset(buf,'\0', BUFSIZE);
		ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &servaddr, &addrlen);
		if (ret < 0) {
	    	perror("recvfrom()"); 
	    }
	    Segment seg(buf, buf + ret);
	    Packet current_packet(seg);
		current_packet.toString();
		switch(current_state) { 
			case SYNWAIT:
				// check for ACK 
				// if not correct, send again
				if ((nfds = select(sockfd+1, &readFds, NULL, NULL, &tv)) == -1) {
           			perror("select");
        		}
        		if (nfds == 0) {
        			retransmit = true;
        			syn_tries++;
        			if (syn_tries == 3) {
        				cerr << "Client could not connect" << endl;
        			}
        		}
				if(current_packet.hasSYN() && current_packet.hasACK() && current_packet.getAckNo() == rcvbuf.getSeqNo() + 1) {
	    			ackNo = current_packet.getSeqNo() + 1;
	    			rcvbuf.setSeqNo((rcvbuf.getSeqNo() + 1) % MAXSEQNO);
	    			current_state = CONNECTED;
	    		}
	    		else {
	    			perror("SYN-ACK invalid");
	    		}
	    		break; 

			case CONNECTED:
				// CHECK CASE WHERE ACK GETS LOST?????????
				if (current_packet.hasFIN()) {
					current_state = CLOSE;
					ackNo = (current_packet.getSeqNo() + 1) % MAXSEQNO; 						
					break;
				}
				// If the packet is in the window
				rcvbuf.insert(current_packet);
				if (ackNo == current_packet.getSeqNo()) {
					ackNo = (current_packet.getSeqNo() + current_packet.getData().size()) % MAXSEQNO;
					for (auto )		
				}
				// store packet in receive buffer 
				break;
			default:
				perror("How did you get here?");
		}

		// Client only acks 
		Packet ack;
		ack.setSeqNo(rcvbuf.getSeqNo());
		ack.setACK();
		ack.setAckNo(ackNo);
		Segment toSend = ack.encode();
		if (state == SYNWAIT) {
			cout << "Sending SYN Packet";
			if (retransmit) {
				cout << " Retransmission";
			}
			cout << endl;
		}
		else if (state == CONNECTED) {
			cout << "Sending ACK Packet " << ackNo << endl;
		}
		if (sendto(sockfd, toSend.data(), toSend.size(), 0 , (struct sockaddr *) &servaddr, addrlen) < 0)
	   	{
	   		perror("sendto(): ACK"); 
	    }
	}

	// Reassemble data by sorting and piecing back together 
	rcvbuf.sortBuffer();
	vector<DataSeqPair> fileBuf = rcvbuf.getBuffer();
	ofstream outfile("file.txt", std::ios::out | std::ios::binary );
	ostream_iterator<uint8_t> oi(outfile, "");
	for (auto i = fileBuf.begin(); i != fileBuf.end(); i++) {
		copy((*i).data.begin(), (*i).data.end(), oi);
	}

    close(sockfd);
    return 0;
}
