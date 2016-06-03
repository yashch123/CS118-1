#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <cstdint> 

#define FIN 0x1
#define SYN 0x2
#define ACK 0x4 

typedef std::vector<uint8_t> Segment;
typedef std::vector<uint8_t> Data;

struct TcpHeader {
public:
	uint16_t seqNo;
 	uint16_t ackNo;
 	uint16_t rcvWin;
 	uint16_t flags = 0;
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
	Packet(Segment encoded);
	void setHeader(TcpHeader header);
	void setData(Data data);
	void setSeqNo(uint16_t seqNo);
	void setAckNo(uint16_t ackNo);
	void setRcvWin(uint16_t rcvWin);
	void setFlags(uint16_t flags);
	int getSeqNo();
	int getAckNo();
	void setSYN(); 
	void setACK(); 
	void setFIN(); 
	bool hasSYN(); 
	bool hasACK(); 
	bool hasFIN(); 
	TcpHeader getHeader();
	Data getData();
	Segment encode();
private:
	TcpHeader m_header;
	Data m_data;
};

//////////////////////////////////////////
/////////// Constructors /////////////////
//////////////////////////////////////////

Packet::Packet() {
	//nothing to do
}

Packet::Packet(Segment encoded) {
	setSeqNo((encoded[0] << 8) 	| encoded[1]);
	setAckNo((encoded[2] << 8) 	| encoded[3]);
	setRcvWin((encoded[4] << 8) | encoded[5]);
	setFlags((encoded[6] << 8) 	| encoded[7]);
	
	m_data.insert(m_data.end(), encoded.begin() + 8, encoded.end());
}

//////////////////////////////////////////
/////////////// Setters  /////////////////
//////////////////////////////////////////

void Packet::setHeader(TcpHeader header) {
	m_header = header;
}

void Packet::setData(Data data) {
	m_data = data;
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

void Packet::setFlags(uint16_t flags) {
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

//////////////////////////////////////////
/////////////// Getters  /////////////////
//////////////////////////////////////////

int Packet::getSeqNo() {
	return m_header.seqNo;
}

int Packet::getAckNo() {
	return m_header.ackNo;
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

Data Packet::getData() {
	return m_data;
}

//////////////////////////////////////////
////// Auxiliary Functions  //////////////
//////////////////////////////////////////

Segment Packet::encode() {
	Segment v;
	uint8_t firstHalf, secondHalf;

	// Sequence Number
	firstHalf = m_header.seqNo >> 8;
	secondHalf = m_header.seqNo;
	v.push_back(firstHalf);
	v.push_back(secondHalf);

	// Ack Number
	firstHalf = m_header.ackNo >> 8;
	secondHalf = m_header.ackNo;
	v.push_back(firstHalf);
	v.push_back(secondHalf);

	// Receive Window
	firstHalf = m_header.rcvWin >> 8;
	secondHalf = m_header.rcvWin;
	v.push_back(firstHalf);
	v.push_back(secondHalf);

	// Flags
	firstHalf = m_header.flags >> 8;
	secondHalf = m_header.flags;
	v.push_back(firstHalf);
	v.push_back(secondHalf);

	// Data
	v.insert(v.end(), m_data.begin(), m_data.end());
	return v;
}

#endif // PACKET_H
