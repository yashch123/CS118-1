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
	Segment getSeg(uint16_t seq); // Not sure what this does
private:
	uint16_t m_seqNo;
	TcpHeader m_header;
	Segment m_seg;
};

class FileReader {
public:
	Segment top();
	void pop();
	bool hasMore();
};

void OutputBuffer::setInitSeq(uint16_t seqNo) {
	m_header.seqNo = seqNo;
	m_seg.reserve(1024);
}

void OutputBuffer::ack(uint16_t ackNo) {
	m_header.ackNo = ackNo;
}

void OutputBuffer::timeout() {
	//???
	// fast retransmit?
	// or poll for timeout? 
}

bool OutputBuffer::hasSpace(uint16_t size) {
	return (m_seg.capacity() - m_seg.size() >= size);
}

void OutputBuffer::insert(Segment seg) {
	m_seg.insert(m_seg.end(), seg.begin(), seg.end());
}

uint16_t OutputBuffer::nextSegSeq() {
	return (m_seqNo + m_seg.size() * 2);
}

Segment OutputBuffer::getSeg(uint16_t) {
	return m_seg;
}


#endif