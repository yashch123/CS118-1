#include <string>
#include <thread>
#include <iostream>

#include <stdio.h> 
#include <string.h> 	// memset, memcpy 
#include <sys/socket.h> // socket API 
#include <netinet/in.h> // sockaddr_in
#include <unistd.h> 	// close()
#include <netdb.h> 		// gethostbyname
#include <stdlib.h> 	// atoi, rand
#include <ctype.h>		// isalpha

#include "client.h"

#define PORTNUM 0
#define BUFLEN 8192
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

	// TODO: Set up TCP connection 
	// 1) SYN w/ random sequence no. 
	// 2) wait for SYN ACK (w/ server's random sequence no. & ack no. = client's + 1)
	// 3) SYN = 0, sequence no. = client's + 1, ack no. = server's + 1, payload possible
	uint16_t buf[BUFLEN];
	socklen_t slen = sizeof(servaddr); 

	uint16_t clientSeqNo = rand() % BUFLEN;
	uint16_t ackToServer;
	while(true) {
		//SYN
		Packet syn;
		syn.setSYN();
		syn.setSeqNo(clientSeqNo);
		vector<uint16_t> synVec = syn.encode();
	   	if (sendto(sockfd, synVec.data(), synVec.size(), 0 , (struct sockaddr *) &servaddr, slen) == -1)
	   	{
	   		perror("sendto(): SYN"); 
	    }
	    memset(buf,'\0', BUFLEN);
	    if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &servaddr, &slen) == -1)
	    {
	    	perror("recvfrom(): SYN-ACK"); 
	    }
	    Packet synResponse(buf);
	    //if valid SYN-ACK, break, otherwise loop
	    if(synResponse.hasSYN() && synResponse.hasACK() && synResponse.getAckNo() == clientSeqNo + 1) {
	    	ackToServer = synResponse.getSeqNo() + 1;
	    	clientSeqNo++;
	    	break;
	    }
	    else {
	    	perror("SYN-ACK invalid");
	    }
	}

	while(true) {
		//ACK
		Packet ack;
		ack.setACK();
		ack.setAckNo(ackToServer);
		vector<uint16_t> ackVec = ack.encode();
		if (sendto(sockfd, ackVec.data(), ackVec.size(), 0 , (struct sockaddr *) &servaddr, slen) == -1)
	   	{
	   		perror("sendto(): ACK"); 
	    }
	    memset(buf,'\0', BUFLEN);
	    if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &servaddr, &slen) == -1)
	    {
	    	perror("recvfrom()"); 
	    }
	    Packet ackResponse(buf);
	   	cout << ackResponse.getSeqNo() << " " << ackResponse.getAckNo() << endl;

	   	//to be changed after we get the rest of this working
	   	for (Segment::const_iterator i = ackResponse.getSegment().begin(); i != ackResponse.getSegment().end(); ++i)
   			cout << *i << ' ';

   		break;
	}




	/*
	unsigned int slen = sizeof(servaddr); 
	while(1){
		cout << "Enter message : "; 
	    gets(message);
         
        //send the message
        if (sendto(sockfd, message, strlen(message) , 0 , (struct sockaddr *) &servaddr, slen) == -1)
        {
            perror("sendto()"); 
        }
         
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &servaddr, &slen) == -1)
        {
        	perror("recvfrom()"); 
        }
         
        puts(buf);
    }
 	*/ 
    close(sockfd);
    return 0;

	// TODO: Receive file from server 
	
	// Max payload: 1024 bytes 

	// TODO: Connection teardown 
	// 1) send FIN 
	// 2) wait for ACK 
	// 3) server closes, wait for FIN 
	// 4) send ACK, wait for timeout, then close connection 
}
