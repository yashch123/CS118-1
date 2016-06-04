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

enum state {
	SYNWAIT,
	CONNECTED, 
	CLOSE
};

/*******************
struct sockaddr_in {
	__uint8_t sin_len; 
	sa_family_t sin_family; 
	in_port_t sin_port; 
	struct in_addr sin_addr; 
	char sin_zero[8]; 
};
********************/

void error(char *msg){
	perror(msg); 
	exit(1); 
}

int main(int argc, char **argv)
{
	// Parse command line arguments 
	if (argc != 3) {
		cerr << "Usage: " << argv[0] << " <SERVER-HOST-OR-IP>" << " PORT-NUMBER" << endl; 
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

	// seed 
	srand(time(NULL));
	ReceivingBuffer rcvbuf;
	rcvbuf.setSeqNo(rand() % MAXSEQNO);
	// uint16_t serverSeqNum;

    int ret;

	/*-----------------Begin TCP Handshake-----------------*/
	// Housekeeping 
	uint8_t buf[BUFSIZE];
	memset(buf,'\0', BUFSIZE);
	socklen_t addrlen = sizeof(servaddr);
	uint16_t ackNo = 0;

	// Create syn 
	state current_state = SYNWAIT; 
	Packet syn_packet; 
	syn_packet.setSYN(); 
	syn_packet.setSeqNo(rcvbuf.getSeqNo());
	Segment syn_encoded = syn_packet.encode(); 
	syn_packet.toString();
	if (sendto(sockfd, syn_encoded.data(), syn_encoded.size(), 0 , (struct sockaddr *) &servaddr, addrlen) < 0) {
	   	perror("sendto()"); 
	}

	// Enter finite state machine loop 
	while(1) {
		if (current_state == CLOSE) {
			// WAIT FOR SOMETIME
			rcvbuf.setSeqNo((rcvbuf.getSeqNo() + 1) % MAXSEQNO);
			Packet finPacket;
			finPacket.setFIN();
			finPacket.setSeqNo(rcvbuf.getSeqNo());
			finPacket.toString();
			Segment finSend = finPacket.encode();
			cerr << "Sending FIN" << endl;
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
	    		ackNo = current_packet.getSeqNo() + current_packet.getData().size() + 1; 	    		
				// store packet in receive buffer 
				break;
			default:
				perror("How did you get here?");
		}

		Packet ack;
		ack.setSeqNo(rcvbuf.getSeqNo());
		ack.setACK();
		ack.setAckNo(ackNo);
		Segment toSend = ack.encode();
		cout << "Sending ACK Packet " << ackNo << endl;
		if (sendto(sockfd, toSend.data(), toSend.size(), 0 , (struct sockaddr *) &servaddr, addrlen) < 0)
	   	{
	   		perror("sendto(): ACK"); 
	    }
	}

	rcvbuf.sortBuffer();
	vector<DataSeqPair> fileBuf = rcvbuf.getBuffer();
	ofstream outfile("file.txt", std::ios::out | std::ios::binary );
	ostream_iterator<uint8_t> oi(outfile, "");
	for (auto i = fileBuf.begin(); i != fileBuf.end(); i++) {
		copy((*i).data.begin(), (*i).data.end(), oi);
	}


	// I (Connor) have only gone to here in client, still need to check all the other stuff below (change types and structure etc)
	// Cool story bro - Anderson 

	// Restructuring: 
	// buffer now deals with bytes, NOT shorts 
	// One single while loop to handle packets 
	// Segment is encoded packet 
	// packet is classful segment 
	// Data and Segment both vector of uint8_t 
	// Should we use a queue instead of a vec?
	// TODO: 
	// 1) Restructure
	// 2) Integrate Receive Buffer into packet 
	// 3) Congestion, flow control, retransmissions, timeout, select 

	/******************* Old code *****************************

	// TODO: Set up TCP connection 
	// 1) SYN w/ random sequence no. 
	// 2) wait for SYN ACK (w/ server's random sequence no. & ack no. = client's + 1)
	// 3) SYN = 0, sequence no. = client's + 1, ack no. = server's + 1, payload possible

	
	while(true) {
		//SYN
		Packet syn;
		syn.setSYN();
		syn.setSeqNo(clientSeqNo);
		Segment synVec = syn.encode();
		for(auto p = synVec.begin(); p != synVec.end(); p++)
            cerr << *p << endl;
	   	if (sendto(sockfd, synVec.data(), sizeof(synVec), 0 , (struct sockaddr *) &servaddr, addrlen) < 0)
	   	{
	   		perror("sendto(): SYN"); 
	    }
	    memset(buf,'\0', BUFSIZE);
	    if (recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &servaddr, &addrlen) < 0)
	    {
	    	perror("recvfrom(): SYN-ACK"); 
	    }
	    Segment newPacket = vector(buf, buf+)
	    Packet synResponse(buf);
	    vector<uint16_t> v = synResponse.encode();
	    //if valid SYN-ACK, break, otherwise loop
	    if(synResponse.hasSYN() && synResponse.hasACK() && synResponse.getAckNo() == clientSeqNo + 1) {
	    	ackToServer = synResponse.getSeqNo() + 1;
	    	clientSeqNo = (clientSeqNo + 1) % MAXSEQNUM;
	    	break;
	    }
	    else {
	    	perror("SYN-ACK invalid");
	    }
	}
	while(true) {
		//ACK
		Packet ack;
		ack.setSeqNo(clientSeqNo);
		ack.setACK();
		ack.setAckNo(ackToServer);
		vector<uint16_t> ackVec = ack.encode();
		cout << "Sending ACK Packet " << ackToServer << endl;
		if (sendto(sockfd, ackVec.data(), sizeof(ackVec), 0 , (struct sockaddr *) &servaddr, addrlen) < 0)
	   	{
	   		perror("sendto(): ACK"); 
	    }
	    memset(buf,'\0', BUFSIZE);
	    int ret = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &servaddr, &addrlen);
	    cerr << "ret = " << ret << endl;
	    if (ret < 0)
	    {
	    	perror("recvfrom()"); 
	    	break;
	    }
	    if (ret == 0)
	    	break;
	    cerr << "Printing out buffer AKA file" << endl;
        Segment payload(buf + 4, buf + 4 + ret);
        for (auto p = buf + 4; p != buf + 4 + ret; p++) {
            cerr << (char) *p;
            cerr << (char) ((*p) >> 8);
        }
        cerr << "START" << endl;
	    Packet ackResponse(buf);
	    for (auto p = ackResponse.getSegment().begin(); p != ackResponse.getSegment().end(); p++)
		    cerr << (char*) *p;
		cerr << endl;
		cerr << "END" << endl;
	    cout << "Receiving data packet " << ackResponse.getSeqNo() << endl;
	   	ofstream outfile("file.txt", std::ios::out | std::ios::binary );
		ostream_iterator<uint16_t> oi(outfile, ""); 
		copy(ackResponse.getSegment().begin(), ackResponse.getSegment().end(), oi);
	}

	***************************** Old Code **********************/ 
    close(sockfd);
    return 0;
}
