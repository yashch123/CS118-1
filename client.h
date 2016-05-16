#ifndef CLIENT_H
#define CLIENT_H

#include "tcpheader.h"

class ReceivingBuffer {
public:
	void setInitSeq(uint16_t seqNo);
	uint16_t insert(uint16_t seqNo, Segment seg);
private:
	uint16_t m_seqNo;
	std::vector<Segment> m_buffer;
};

void ReceivingBuffer::setInitSeq(uint16_t seqNo) {
	m_seqNo = seqNo;
	m_buffer.reserve(64);
}

uint16_t ReceivingBuffer::insert(uint16_t seqNo, Segment seg) {
	if(seqNo - m_seqNo >= m_buffer.capacity()) {
		return 0;
	}
	m_buffer[seqNo - m_seqNo] = seg;
}

#endif