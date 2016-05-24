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
	Packet(uint16_t *arr);
	void setHeader(TcpHeader h);
	void setSegment(Segment seg);
	void setSeqNo(uint16_t seqNo);
	void setAckNo(uint16_t ackNo);
	void setRcvWin(uint16_t rcvWin);
	void setFlags(uint8_t flags);
	int getSeqNo();
	int getAckNo();
	void setSYN(); 
	void setACK(); 
	void setFIN(); 
	bool hasSYN(); 
	bool hasACK(); 
	bool hasFIN(); 
	TcpHeader getHeader();
	Segment getSegment();
	std::vector<uint16_t> encode();
	void appendToSegment(Segment s);
private:
	TcpHeader m_header;
	Segment m_seg;
};

Packet::Packet() {
	//nothing to do
}

Packet::Packet(uint16_t *arr) {
	int size = sizeof(arr)/sizeof(arr[0]);
	setSeqNo(arr[0]);
	setAckNo(arr[1]);
	setRcvWin(arr[2]);
	setFlags(arr[3]);
	for(int i = 4; i < size; i++) {
		appendToSegment(arr[i]);
	}
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
int Packet::getSeqNo() {
	return m_header.getSeqNo;
}

int Packet::getAckNo() {
	return m_header.ackNo;
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

bool Packet::hasSYN() {
	return (m_header.flags & SYN);
}

bool Packet::hasACK() {
	return (m_header.flags & ACK);
}

bool Packet::hasFIN() {
	return (m_header.flags & FIN);
}

TcpHeader Packet::getHeader() {
	return m_header;
}

Segment Packet::getSegment() {
	return m_seg;
}

std::vector<uint16_t> Packet::encode() {
	std::vector<uint16_t> v;
	v.push_back(m_header.seqNo);
	v.push_back(m_header.ackNo);
	v.push_back(rcvWin);
	uint16_t f = m_header.flags | 0x0000;
	v.push_back(f);
	v.insert(v.end(), m_seg.begin(), m_seg.end());
	return v;
}

void Packet::appendToSegment(Segment s) {
	m_seg.insert(m_seg.end(), s.begin(), s.end());
}
 #endif // PACKET_H}
