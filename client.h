#ifndef CLIENT_H
#define CLIENT_H

#include "packet.h"

// Used to keep track in the Receiving Buffer
struct DataSeqPair
{
	Data data;
	uint16_t seq;
};


class ReceivingBuffer {
public:
	void setInitSeq(uint16_t seqNo);
	uint16_t insert(uint16_t seqNo, Packet p);
	std::vector<DataSeqPair> getBuffer();
private:
	// Initial sequence number
	uint16_t m_seqNo;
	// Buffer is a vector of packet data and sequence number pairs
	std::vector<DataSeqPair> m_buffer;
};

// Set up the client with seqNo
void ReceivingBuffer::setInitSeq(uint16_t seqNo) {
	m_seqNo = seqNo;
}

// Inserts packet into buffer as DataSeqPair and returns ACK number
uint16_t ReceivingBuffer::insert(uint16_t seqNo, Packet p) {
	// Set up a DataSeqPair
	DataSeqPair toInsert;
	toInsert.seq = p.getSeqNo();
	toInsert.data = p.getData();
	m_buffer.push_back(toInsert);

	// TODO Deal with out of order packets	
	
}

std::vector<DataSeqPair> getBuffer() {
	return m_buffer;
}

#endif