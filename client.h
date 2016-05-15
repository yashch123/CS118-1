#ifndef CLIENT_H
#define CLIENT_H

#include "tcpheader.h"

class ReceivingBuffer {
public:
	void setInitSeq(uint16_t seqNo);
	uint16_t insert(uint16_t seqNo, Segment seg);
private:
	uint16_t m_seqNo;
	Segment m_seg;
};

void ReceivingBuffer::setInitSeq(uint16_t seqNo) {
	m_seqNo = seqNo;
}

uint16_t ReceivingBuffer::insert(uint16_t seqNo, Segment seg) {
	m_seg = seg;
	return seqNo + (seg.size() * 2);
}




#endif