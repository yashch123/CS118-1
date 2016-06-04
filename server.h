#ifndef SERVER_H
#define SERVER_H

#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <queue>
#include <unordered_map>

#include "packet.h"

#define MAXPAYLOAD 1024
#define MAXSEQNO 30720 // in bytes 

class OutputBuffer {
public:
	void setInitSeq(uint16_t seqNo);
	void ack(uint16_t ackNo);
	void timeout();
	bool hasSpace(uint16_t size = 1024);
	void insert(Packet p);
	uint16_t nextSegSeq();
	Segment getSeg(uint16_t seq); // Not sure what this does
private:
	uint16_t m_seqNo;	//sequence number
	uint16_t m_currentWinSize;	//current number of bytes in the network
	uint16_t m_maxWinSize;	//maximum number of bytes allowed in the current congestion window
	std::unordered_map<uint16_t, Segment> m_map;	//its a map its a map its a map its a map
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
}

void OutputBuffer::ack(uint16_t ackNo) {
	m_map.erase(ackNo);
}

void OutputBuffer::timeout() {
	//resets window variables
}

bool OutputBuffer::hasSpace(uint16_t size) {
	return (m_maxWinSize - m_currentWinSize < size);
}

void OutputBuffer::insert(Packet p) {
	//main loop must call hasSpace before to avoid congestion
	Segment s = p.encode();
	m_map[p.getSeqNo()] = s;
	m_seqNo = (m_seqNo + p.getData().size()) % MAXSEQNO;
}

uint16_t OutputBuffer::nextSegSeq() {
	return m_seqNo;
}

Segment OutputBuffer::getSeg(uint16_t seq) {
	return m_map[seq];
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