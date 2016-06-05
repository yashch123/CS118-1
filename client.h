#ifndef CLIENT_H
#define CLIENT_H

#include <algorithm> 
#include "packet.h"

// Used to keep track in the Receiving Buffer
struct DataSeqPair
{
	Data data;
	unsigned int seq;
};

bool compareForSort(const struct DataSeqPair& l, const struct DataSeqPair& r) { 
	return l.seq < r.seq; 
} 

class ReceivingBuffer {
public:
	ReceivingBuffer(); 
	void setSeqNo(uint16_t seqNo);
	uint16_t getSeqNo() { return m_seqNo; }
	uint16_t insert(Packet p);
	std::vector<DataSeqPair> getBuffer();
	void sortBuffer();
private:
	// Initial sequence number
	uint16_t m_seqNo;
	// Buffer is a vector of packet data and sequence number pairs
	//uint16_t m_cumSeqNo; 
	int m_round; // keeps track of round number since sequence numbers may become non-unique 
	std::vector<DataSeqPair> m_buffer;
};

ReceivingBuffer::ReceivingBuffer() { 
	m_round = 0; 
}
// Set up the client with seqNo
void ReceivingBuffer::setSeqNo(uint16_t seqNo) {
	m_seqNo = seqNo;
}

// Inserts packet into buffer as DataSeqPair and returns ACK number
uint16_t ReceivingBuffer::insert(Packet p) {
	// Set up a DataSeqPair
	DataSeqPair toInsert;
	toInsert.seq = p.getSeqNo() + (m_round * MAXSEQNO);	// MAXSEQNO = 30720 
	toInsert.data = p.getData();
	if (toInsert.data.size() + p.getSeqNo() > MAXSEQNO)
		m_round++;
	m_buffer.push_back(toInsert);
	m_buffer.sortBuffer();
	for (int k = 0; k < m_buffer.size() - 1; k++) {
		if (m_buffer[k].seq + m_buffer[k].data.size() != m_buffer[k+1].seq)
			break;
	}
	return (m_buffer[k].seq + m_buffer.data.size()) % MAXSEQNO;
}

std::vector<DataSeqPair> ReceivingBuffer::getBuffer() {
	return m_buffer;
}

void ReceivingBuffer::sortBuffer() {
	std::sort(m_buffer.begin(), m_buffer.end(), compareForSort);
}


#endif