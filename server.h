#ifndef SERVER_H
#define SERVER_H

#include "packet.h"

class OutputBuffer {
public:
	void setInitSeq(uint16_t seqNo);
	void ack(uint16_t ackNo);
	void timeout();
	bool hasSpace(uint16_t size = 1024);
	void insert(Segment seg);
	uint16_t nextSegSeq();
	Segment getSeg(uint16_t);
private:
	uint16_t m_seqNo;
};

class FileReader {
public:
	Segment top();
	void pop();
	bool hasMore();
};

void OutputBuffer::setInitSeq(uint16_t seqNo) {
	m_header.seqNo = seqNo;
}

void OutputBuffer::ack(uint16_t ackNo) {
	m_header.ackNo = ackNo;
}

void OutputBuffer::timeout() {

}
#endif