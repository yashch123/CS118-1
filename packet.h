#ifndef PACKET_H
#define PACKET_H

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

class Packet {
public:
	Packet(uint16_t seqNo, uint16_t ackNo, uint16_t rcvWin, uint8_t flags, Segment seg);
	void setSeqNo(uint16_t seqNo);
	void setAckNo(uint16_t ackNo);
	void setRcvWin(uint16_t rcvWin);
	void setFlags(uint8_t flags);
	void setSegment(Segment seg);
	TcpHeader getHeader();
	Segment getSegment();
private:
	TcpHeader m_header;
	Segment m_seg;
};

Packet::Packet(uint16_t seqNo, uint16_t ackNo, uint16_t rcvWin, uint8_t flags, Segment seg) {
	m_header.seqNo = seqNo;
	m_header.ackNo = ackNo;
	m_header.rcvWin = rcvWin;
	m_header.flags = flags;
	m_seg = seg;
}

void Packet::setHeader(TcpHeader header) {
	m_header = header;
}

void Packet::setSegment(Segment seg) {
	m_seg = seg;
}

TcpHeader Packet::getHeader() {
	return m_header;
}

Segment Packet::getSegment() {
	return m_seg;
}



 #endif