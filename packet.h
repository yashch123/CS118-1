#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <cstdint> 

#define FIN 0x1
#define SYN 0x2
#define ACK 0x4 

typedef std::vector<uint16_t> Segment;

struct TcpHeader {
public:
	uint16_t seqNo;
 	uint16_t ackNo;
 	uint16_t rcvWin;
 	uint8_t reserved;
 	uint8_t flags = 0;
 };

/*******************************
Flag format of TCP Header
7 6 5 4 3 2 1 0 
 | | | | |A|S|F
 		  C|Y|I
 		  K|N|N
********************************/ 
 		  
class Packet {
public:
	Packet();
	Packet(TcpHeader h, Segment s);
	Packet(uint16_t seqNo, uint16_t ackNo, uint16_t rcvWin, uint8_t flags, Segment seg);
	void setHeader(TcpHeader h);
	void setSegment(Segment seg);
	void setSeqNo(uint16_t seqNo);
	void setAckNo(uint16_t ackNo);
	void setRcvWin(uint16_t rcvWin);
	void setFlags(uint8_t flags);
	void setSYN(); 
	void setACK(); 
	void setFIN(); 
	TcpHeader getHeader();
	Segment getSegment();
private:
	TcpHeader m_header;
	Segment m_seg;
};

Packet::Packet() {
	//nothing to do
}

Packet::Packet(TcpHeader h, Segment s) {
	m_header = h;
	m_seg = s;
}

Packet::Packet(uint16_t seqNo, uint16_t ackNo, uint16_t rcvWin, uint8_t flags, Segment seg) {
	m_header.seqNo = seqNo;
	m_header.ackNo = ackNo;
	m_header.rcvWin = rcvWin;
	m_header.flags = flags;
	m_seg = seg;
}

void Packet::setHeader(TcpHeader h) {
	m_header = h;
}

void Packet::setSegment(Segment seg) {
	m_seg = seg;
}

void Packet::setSeqNo(uint16_t seqNo) {
	m_header.seqNo = seqNo;
}

void Packet::setAckNo(uint16_t ackNo) {
	m_header.ackNo = ackNo;
}

void Packet::setRcvWin(uint16_t rcvWin) {
	m_header.rcvWin = rcvWin;
}

void Packet::setFlags(uint8_t flags) {
	m_header.flags = flags;
}

void Packet::setSYN() { 
	m_header.flags |= SYN; 
}

void Packet::setACK() { 
	m_header.flags |= ACK; 
}

void Packet::setFIN() { 
	m_header.flags |= FIN; 
}

TcpHeader Packet::getHeader() {
	return m_header;
}

Segment Packet::getSegment() {
	return m_seg;
}


 #endif // PACKET_H