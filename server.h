#ifndef SERVER_H
#define SERVER_H

#include "tcpheader.h"

class OutputBuffer {
public:
	void setInitSeq(uint16_t seqNo);
	void ack(uint16_t ackNo);
	void timeout();
	bool hasSpace(uint16_t size = 1024);
	void insert(Segment seg);
	uint16_t nextSegSeq();
	Segment getSeg(uint16_t);
};

class FileReader {
public:
	Segment top();
	void pop();
	bool hasMore();
};
#endif