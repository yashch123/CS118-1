#include <string>
#include <thread>
#include <iostream>

#include "client.h"

using namespace std;

int main()
{
	// TODO: Parse command line arguments 

	// TODO: Set up TCP connection 
	// 1) SYN w/ random sequence no. 
	// 2) wait for SYN ACK (w/ server's random sequence no. & ack no. = client's + 1)
	// 3) SYN = 0, sequence no. = client's + 1, ack no. = server's + 1, payload possible 

	// TODO: Receive file from server 
	
	// Max payload: 1024 bytes 

	// TODO: Connection teardown 
	// 1) send FIN 
	// 2) wait for ACK 
	// 3) server closes, wait for FIN 
	// 4) send ACK, wait for timeout, then close connection 
}
