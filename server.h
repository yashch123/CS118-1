#ifndef SERVER_H
#define SERVER_H

#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <queue>
#include <unordered_map>

#include "packet.h"

enum state {
    SLOWSTART,
    AVOIDANCE,
    FASTRET
};

class OutputBuffer {
public:
	void setInitSeq(uint16_t seqNo);
	void ack(uint16_t ackNo);
	void timeout();
	bool hasSpace(uint16_t size = 1024);
	uint16_t insert(Packet p);
	uint16_t nextSegSeq();
	Segment getSeg(uint16_t seq); // Not sure what this does
	bool isEmpty();
	void toString();
private:
	uint16_t m_seqNo;	//sequence number
	uint16_t m_currentWinSize;	//current number of bytes in the network
	uint16_t m_maxWinSize;	//maximum number of bytes allowed in the current congestion window
	std::unordered_map<uint16_t, Segment> m_map;	//its a map its a map its a map its a map
	state m_state;
};

class FileReader {
public:
	FileReader(std::string filename);
	Data top();
	void pop();
	bool hasMore();
private:
	std::queue<Data> m_buf;
};

void OutputBuffer::setInitSeq(uint16_t seqNo) {
	m_seqNo = seqNo;
	m_currentWinSize = 0;
	m_maxWinSize = 1024;
	m_state = SLOWSTART;
}

void OutputBuffer::ack(uint16_t ackNo) {
	//std::cerr << "Removing from buffer: " << ackNo << std::endl;
	std::cout << "Receiving ACK packet " << ackNo << std::endl;
	if(m_map.find(ackNo) != m_map.end()) {
		m_currentWinSize -= (m_map[ackNo].size() - 8);
		m_map.erase(ackNo);
	}
	switch(m_state) {
		case SLOWSTART:
			m_maxWinSize *= 2;
			if(m_maxWinSize > SSTHRESH) {
				m_state = AVOIDANCE;
			}
			break;
		case AVOIDANCE:
			m_maxWinSize += MAXPAYLOAD;
			break;
		case FASTRET:
			std::cerr << "Not implemented yet" << std::endl;
			break;
		default:
			std::cerr << "Error: invalid state" << std::endl;
	}
	if(m_maxWinSize > MAXSEQNO/2) {
		m_maxWinSize = MAXSEQNO/2;
	}
}

void OutputBuffer::timeout() {
	//resets window variables
}

bool OutputBuffer::hasSpace(uint16_t size) {
	return ((m_maxWinSize - m_currentWinSize) >= size);
}

uint16_t OutputBuffer::insert(Packet p) {
	//main loop must call hasSpace before to avoid congestion
	p.setSeqNo(nextSegSeq());
	p.setRcvWin(m_maxWinSize);

	int ackNo;
	if(p.hasSYN() || p.hasFIN()) {
		ackNo = (nextSegSeq() + 1) % MAXSEQNO;
		m_seqNo = (m_seqNo + 1) % MAXSEQNO;
	}
	else {
		ackNo = (nextSegSeq() + p.getData().size()) % MAXSEQNO;
		//if p is a normal data packet, print message
		if(!p.hasACK()) {
			std::cout << "Sending data packet " << nextSegSeq() << " " << m_maxWinSize << " " << SSTHRESH;
			//retransmission?
			std::cout << std::endl;
		}
		m_seqNo = (m_seqNo + p.getData().size()) % MAXSEQNO;
	}
	//std::cerr << "Adding to buffer: " << ackNo << std::endl;

	Segment s = p.encode();
	m_map[ackNo] = s;
	m_currentWinSize += (p.getData().size());
	return ackNo;
}

uint16_t OutputBuffer::nextSegSeq() {
	return m_seqNo;
}

Segment OutputBuffer::getSeg(uint16_t seq) {
	return m_map[seq];
}

bool OutputBuffer::isEmpty() {
	return m_map.empty();
}

void OutputBuffer::toString() {
	std::cerr << "Max window size: " << m_maxWinSize << "\t" << "Current window size: " << m_currentWinSize << std::endl << "Diff: " << (m_maxWinSize - m_currentWinSize) << std::endl;
	std::cerr << "Buffer contents: " << std::endl;
	std::cerr << "Size: " << m_map.size() << std::endl;
	for(std::unordered_map<uint16_t, Segment>::iterator i = m_map.begin(); i != m_map.end(); i++) {
		std::cerr << i->first << "\t" <<  std::endl;
	}
	std::cerr << std::endl;
}

FileReader::FileReader(std::string filename) {
	int fd = open(filename.c_str(), O_RDONLY);
	uint8_t buf[MAXPAYLOAD];
    int ret;
    while ((ret = read(fd, buf, sizeof(buf))) != 0) {
        Data v(buf, buf + ret);
        m_buf.push(v);
        memset(buf,'\0', MAXPAYLOAD);
    }
    close(fd);
}

Data FileReader::top() {
	return m_buf.front();
}

void FileReader::pop() {
	m_buf.pop();
}

bool FileReader::hasMore() {
	return !m_buf.empty();
}

#endif