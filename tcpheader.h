#ifndef TCPHEADER_H
#define TCPHEADER_H

#include <vector>

typedef std::vector<uint16_t> Segment;

struct TcpHeader {
public:
	uint16_t seqNo;
 	uint16_t ackNo;
 	uint16_t rcvWin;
 	uint8_t reserved;
 	uint8_t flags;
 };
 #endif