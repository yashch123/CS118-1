#include <string>
#include <thread>
#include <iostream>
#include <iterator>

#include <stdio.h> 
#include <string.h> 	// memset, memcpy 
#include <sys/socket.h> // socket API 
#include <netinet/in.h> // sockaddr_in
#include <unistd.h> 	// close()
#include <netdb.h> 		// gethostbyname
#include <stdlib.h> 	// atoi, rand
#include <ctype.h>		// isalpha
#include <fstream>		// ofstream
#include <fcntl.h>

#include "client.h"

#define PORTNUM 0
#define BUFLEN 1032
#define MAXSEQNUM 30720
using namespace std;

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

	cerr << "gethostbyname:" << hp->h_addr_list[0] << endl;

	// Create client socket
	int sockfd; 
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		perror("socket creation failed"); 
		exit(1); 
	}
	//fcntl(sockfd, F_SETFL, O_NONBLOCK);

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

	// TODO: Set up TCP connection 
	// 1) SYN w/ random sequence no. 
	// 2) wait for SYN ACK (w/ server's random sequence no. & ack no. = client's + 1)
	// 3) SYN = 0, sequence no. = client's + 1, ack no. = server's + 1, payload possible
	uint16_t buf[BUFLEN];
	socklen_t slen = sizeof(servaddr); 

	uint16_t clientSeqNo = rand() % MAXSEQNUM;
	uint16_t ackToServer;
	cerr << "Entering SYN while loop" << endl;
	while(true) {
		//SYN
		Packet syn;
		syn.setSYN();
		syn.setSeqNo(clientSeqNo);
		vector<uint16_t> synVec = syn.encode();
		for(auto p = synVec.begin(); p != synVec.end(); p++)
            cerr << *p << endl;
	   	if (sendto(sockfd, synVec.data(), sizeof(synVec), 0 , (struct sockaddr *) &servaddr, slen) == -1)
	   	{
	   		perror("sendto(): SYN"); 
	    }
	    memset(buf,'\0', BUFLEN);
	   	cerr << "Sendto Worked... Recvfrom about to be called" << endl;
	    if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &servaddr, &slen) == -1)
	    {
	    	perror("recvfrom(): SYN-ACK"); 
	    }
	    Packet synResponse(buf);
	    cerr << "printing out synResponse encoded" << endl;
	    vector<uint16_t> v = synResponse.encode();
	    for(auto p = v.begin(); p != v.end(); p++)
            cerr << *p << endl;
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
	cerr << "Sending ACK" << endl;
	while(true) {
		//ACK
		Packet ack;
		ack.setSeqNo(clientSeqNo);
		ack.setACK();
		ack.setAckNo(ackToServer);
		vector<uint16_t> ackVec = ack.encode();
		cout << "Sending ACK Packet " << ackToServer << endl;
		if (sendto(sockfd, ackVec.data(), sizeof(ackVec), 0 , (struct sockaddr *) &servaddr, slen) == -1)
	   	{
	   		perror("sendto(): ACK"); 
	    }
	    memset(buf,'\0', BUFLEN);
	    cerr << "We made it fam" << endl;
	    cerr << "About to receive data" << endl;
	    int ret = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &servaddr, &slen);
	    cerr << "ret = " << ret << endl;
	    if (ret == -1)
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

    close(sockfd);
    return 0;
}
