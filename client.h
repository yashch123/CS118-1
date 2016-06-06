#ifndef CLIENT_H
#define CLIENT_H

#include <algorithm> 
#include "packet.h"

// Used to keep track in the Receiving Buffer
struct DataSeqPair
{
	Data data;
	unsigned int seq;
    int round; 
};

bool compareForSort(const struct DataSeqPair& l, const struct DataSeqPair& r) { 
	return l.seq < r.seq; 
} 

class ReceivingBuffer {
public:
	ReceivingBuffer(); 
	void setSeqNo(uint16_t seqNo);
	uint16_t getSeqNo() { return m_seqNo; }
	void insert(Packet p);
	std::vector<DataSeqPair> getBuffer();
	void sortBuffer();
	uint16_t getExpectedSeqNo(); 
	void setExpectedSeqNo(uint16_t expectedSeqNo);
private:
	// Initial sequence number
	uint16_t m_seqNo;
	uint16_t m_expectedSeqNo; // expected sequence number from server
    uint16_t m_last_unsorted_index;  
	// Buffer is a vector of packet data and sequence number pairs
	//uint16_t m_cumSeqNo; 
	int m_round; // keeps track of round number since sequence numbers may become non-unique 
	std::vector<DataSeqPair> m_buffer;
};

ReceivingBuffer::ReceivingBuffer() { 
	m_round = 0; 
    m_expectedSeqNo = 0;
    m_last_unsorted_index = 0;
}
// Set up the client with seqNo
void ReceivingBuffer::setSeqNo(uint16_t seqNo) {
	m_seqNo = seqNo;
}

// Inserts packet into buffer as DataSeqPair and returns ACK number
void ReceivingBuffer::insert(Packet p) {
    bool already_in_buffer = false; 
    // Set up a DataSeqPair
    unsigned int k;
    DataSeqPair toInsert;
    toInsert.seq = p.getSeqNo() + (m_round * MAXSEQNO); // MAXSEQNO = 30720
    toInsert.data = p.getData();
    toInsert.round = m_round;

    for(k = 0; k < m_buffer.size(); k++){
        if (toInsert.seq == m_buffer[k].seq)
            already_in_buffer = true; 
    }

    if (toInsert.data.size() + p.getSeqNo() > MAXSEQNO)
        m_round++;
    if (!already_in_buffer)
        m_buffer.push_back(toInsert);
    sortBuffer();
    // std::cerr << "In Insert" << std::endl;

    // Go through receive buffer 
    // If I find a matching expected sequence number, I want to increase my expected sequence number according to payload 
    // continue checking for that one just in case its there 

    for(k = m_last_unsorted_index; k < m_buffer.size(); k++) {
        // is there a match to bottom of window 
    	if ((m_buffer[k].seq) == (m_expectedSeqNo + (m_buffer[k].round * MAXSEQNO))) {
    		if (m_buffer[k].data.size() == 0) {
                    m_expectedSeqNo = (m_expectedSeqNo + 1) % MAXSEQNO;
            }
            else {
                // std::cerr << "Insert - Setting expected ack no " << (m_expectedSeqNo + m_buffer[k].data.size()) % MAXSEQNO << std::endl;
                m_expectedSeqNo = (m_expectedSeqNo + m_buffer[k].data.size()) % MAXSEQNO;
                m_last_unsorted_index = k;

                // search for first unacked 
                for(int j = m_last_unsorted_index; j < m_buffer.size(); j++) {
                    if ((m_buffer[j].seq) == (m_expectedSeqNo + (m_buffer[j].round * MAXSEQNO)))
                        m_expectedSeqNo = (m_expectedSeqNo + m_buffer[k].data.size()) % MAXSEQNO;
                    else 
                        return; 
                }
            }
    	}
    }

    std::cerr << "We ran off the FUCKING end" << std::endl;
}

std::vector<DataSeqPair> ReceivingBuffer::getBuffer() {
	return m_buffer;
}

void ReceivingBuffer::sortBuffer() {
	std::sort(m_buffer.begin(), m_buffer.end(), compareForSort);
}

uint16_t ReceivingBuffer::getExpectedSeqNo() { 
	return m_expectedSeqNo; 
}

void ReceivingBuffer::setExpectedSeqNo(uint16_t expectedSeqNo) { 
	m_expectedSeqNo = expectedSeqNo; 
}
#endif