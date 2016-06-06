#ifndef SERVER_H
#define SERVER_H

#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>	//clock

#include <string>
#include <queue>
#include <unordered_map>

#include "packet.h"

enum mode {
    SLOWSTART,
    AVOIDANCE,
    FASTRET
};

struct timeDataPair {
	clock_t time;
	Segment seg;
};

class OutputBuffer {
public:
	void setInitSeq(uint16_t seqNo);
	void ack(uint16_t ackNo);
	void timeout();
	bool hasSpace(uint16_t size = 1024);
	uint16_t insert(Packet p);
	uint16_t nextSegSeq();
	Segment getSeg(uint16_t seq);
	bool isEmpty();
	void toString();
	std::vector<Segment> poll();
private:
	uint16_t m_seqNo;	//sequence number
	uint16_t m_currentWinSize;	//current number of bytes in the network
	uint16_t m_maxWinSize;	//maximum number of bytes allowed in the current congestion window
	std::unordered_map<uint16_t, timeDataPair> m_map;	//its a map its a map its a map its a map
	mode m_mode;
	uint16_t m_ssthresh;
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
	m_mode = SLOWSTART;
	m_ssthresh = 30720;
}

void OutputBuffer::ack(uint16_t ackNo) {
	//std::cerr << "Removing from buffer: " << ackNo << std::endl;
	std::cout << "Receiving ACK packet " << ackNo << std::endl;
	if(m_map.find(ackNo) != m_map.end()) {
		m_currentWinSize -= (m_map[ackNo].seg.size() - 8);
		m_map.erase(ackNo);
	}
	switch(m_mode) {
		case SLOWSTART:
			m_maxWinSize *= 2;
			if(m_maxWinSize > m_ssthresh) {
				m_mode = AVOIDANCE;
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
	m_ssthresh = m_maxWinSize/2;
	m_maxWinSize = 1024;
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
			std::cout << "Sending data packet " << nextSegSeq() << " " << m_maxWinSize << " " << m_ssthresh;
			//retransmission?
			std::cout << std::endl;
		}
		m_seqNo = (m_seqNo + p.getData().size()) % MAXSEQNO;
	}

	timeDataPair pair;
	pair.time = clock();
	pair.seg = p.encode();

	m_map[ackNo] = pair;
	m_currentWinSize += (p.getData().size());
	return ackNo;
}

uint16_t OutputBuffer::nextSegSeq() {
	return m_seqNo;
}

Segment OutputBuffer::getSeg(uint16_t seq) {
	return m_map[seq].seg;
}


// 	Main loop must retransmit all Segments in returned vector from this function immediately after calling
//	polls OutputBuffer to timed-out packets, resets their timers, prints message, and returns segments in vector
std::vector<Segment> OutputBuffer::poll() {
	std::vector<Segment> ret;
	for(std::unordered_map<uint16_t, timeDataPair>::iterator i = m_map.begin(); i != m_map.end(); i++) {
		clock_t begin = i->second.time;
		unsigned int timeWaiting = (clock() - begin)/CLOCKS_PER_SEC * 1000000;
		if(timeWaiting > RTO) {
			ret.push_back(i->second.seg);
			i->second.time = clock();
			Packet p(i->second.seg);
			std::cout << "Sending data packet " << p.getSeqNo() << " " << m_maxWinSize << " " << m_ssthresh << " Retransmission" << std::endl;
		}
	}
	return ret;
}

bool OutputBuffer::isEmpty() {
	return m_map.empty();
}

void OutputBuffer::toString() {
	std::cerr << "Max window size: " << m_maxWinSize << "\t" << "Current window size: " << m_currentWinSize << std::endl << "Diff: " << (m_maxWinSize - m_currentWinSize) << std::endl;
	std::cerr << "Buffer contents: " << std::endl;
	std::cerr << "Size: " << m_map.size() << std::endl;
	for(std::unordered_map<uint16_t, timeDataPair>::iterator i = m_map.begin(); i != m_map.end(); i++) {
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