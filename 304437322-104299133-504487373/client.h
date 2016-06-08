#ifndef CLIENT_H
#define CLIENT_H

#include <algorithm> 
#include <map>
#include <cmath>

#include "packet.h"

class ReceivingBuffer {
public:
    ReceivingBuffer(); 
    void setSeqNo(uint16_t seqNo);
    uint16_t getSeqNo() { return m_seqNo; }
    void insert(Packet p);
    std::map<unsigned int, Data> getBuffer();
    void removeDups(); 
    uint16_t getExpectedSeqNo(); 
    void setExpectedSeqNo(uint16_t expectedSeqNo);
    void printCumulative(uint16_t seqNo);
private:
    // Initial sequence number
    uint16_t m_seqNo;
    uint16_t m_expectedSeqNo; // expected sequence number from server
    uint16_t m_last_unsorted_index;  
    // Buffer is a vector of packet data and sequence number pairs
    //uint16_t m_cumSeqNo; 
    int m_round; // keeps track of round number since sequence numbers may become non-unique 
    std::map<unsigned int, Data> m_buffer;
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

//  inserts data chunk into buffer and adjusts m_expectedSeqNo if corresponding data found
void ReceivingBuffer::insert(Packet p) {
    if(p.getSeqNo() < m_expectedSeqNo) {
        return;
    }
    unsigned int key;
    int diff = std::abs(p.getSeqNo() - m_expectedSeqNo);
    if(diff <= MAXSEQNO/2) {
        key = p.getSeqNo() + (m_round * MAXSEQNO); // MAXSEQNO = 30720;
    }
    else {
        if(m_expectedSeqNo >= MAXSEQNO/2) {
            key = p.getSeqNo() + ((m_round + 1) * MAXSEQNO);
        }
        else {
            key = p.getSeqNo() + ((m_round - 1) * MAXSEQNO);
        }
    }
    //std::cerr << "The key is " << key << std::endl; 
    if(m_buffer.find(key) != m_buffer.end()) {
        //std::cerr << "Packet ignored: seqNo " << key << std::endl;
        return; //do not overwrite existing entries
    }
    Data data = p.getData();
    m_buffer[key] = data;

    unsigned int expectedCumulative = m_expectedSeqNo + (m_round * MAXSEQNO);
    while(m_buffer.find(expectedCumulative) != m_buffer.end()) {
        // std::cerr << "Increasing m_expectedSeqNo from " << expectedCumulative << "(" << expectedCumulative % MAXSEQNO << ")";
        if(m_buffer[expectedCumulative].size() + m_expectedSeqNo > MAXSEQNO) {
            m_round++;
        }
        expectedCumulative = expectedCumulative + m_buffer[expectedCumulative].size();
        m_expectedSeqNo = expectedCumulative % MAXSEQNO;
        // std::cerr << " to " << expectedCumulative << "(" << expectedCumulative % MAXSEQNO << ")" << std::endl;
    }

    /*int roundToUse = m_round;
    unsigned int B = p.getSeqNo() + m_round * MAXSEQNO;
    unsigned int A = m_expectedSeqNo + m_round * MAXSEQNO;
    if (m_round == 0 && (abs(A - B) > MAXWINDOW))
        roundToUse++;
    unsigned int key = p.getSeqNo() + (roundToUse * MAXSEQNO); // MAXSEQNO = 30720;
    std::cerr << "The key is " << key << std::endl; 
    if(m_buffer.find(key) != m_buffer.end()) {
        //std::cerr << "Packet ignored: seqNo " << key << std::endl;
        return; //do not overwrite existing entries
    }
    Data data = p.getData();
    m_buffer[key] = data;

    unsigned int expectedCumulative = m_expectedSeqNo + (m_round * MAXSEQNO);
    bool increaseRound = false;
    while(m_buffer.find(expectedCumulative) != m_buffer.end()) {
        // std::cerr << "Increasing m_expectedSeqNo from " << expectedCumulative << "(" << expectedCumulative % MAXSEQNO << ")";
        if(m_buffer[expectedCumulative].size() + m_expectedSeqNo > MAXSEQNO) {
            //std::cerr << "Round increased from " << m_round << " to " << m_round + 1 << std::endl;
            increaseRound = true;
        }
        expectedCumulative = expectedCumulative + m_buffer[expectedCumulative].size();
        m_expectedSeqNo = expectedCumulative % MAXSEQNO;
        // std::cerr << " to " << expectedCumulative << "(" << expectedCumulative % MAXSEQNO << ")" << std::endl;
    }
    if(increaseRound) {
        m_round++;
    }*/
}

std::map<unsigned int, Data> ReceivingBuffer::getBuffer() {
    return m_buffer;
}


uint16_t ReceivingBuffer::getExpectedSeqNo() { 
    return m_expectedSeqNo; 
}

void ReceivingBuffer::setExpectedSeqNo(uint16_t expectedSeqNo) { 
    m_expectedSeqNo = expectedSeqNo; 
}

void ReceivingBuffer::printCumulative(uint16_t seqNo) {
    std::cerr << seqNo + m_round * MAXSEQNO << std::endl;
}
#endif