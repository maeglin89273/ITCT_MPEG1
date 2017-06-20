//
// Created by maeglin89273 on 2017/6/15.
//


#include "BitBuffer.h"

BitBuffer::BitBuffer(char *fileName) {
    this->buffer = nullptr;
    this->loadSuccess = loadFile(fileName);
    this->bitIdx = 0;
}

bool BitBuffer::loadFile(char *fileName) {
    std::ifstream mpegFile;
    mpegFile.open(fileName, std::ios::in | std::ios::binary);

    this->size = estimateFileSize(mpegFile);
    if (this->size == 0) {
        std::cout << "file " << fileName << " not found" << std::endl;
        return false;
    }
    std::cout << "file size: " << this->size << " bytes" << std::endl;

    this->buffer = new byte[this->size];
    mpegFile.read((char *) this->buffer, this->size);
    mpegFile.close();
    return true;

}

unsigned long BitBuffer::estimateFileSize(std::ifstream &file) {
    long begin = file.tellg();
    file.seekg(0, std::ios_base::end);
    long end = file.tellg();
    file.seekg(0, std::ios_base::beg);
    return (buf_size_t) (end - begin);
}

BitBuffer::~BitBuffer() {
    if (buffer != nullptr) {
        delete[] this->buffer;
    }

}

// return the last byte of the start code, but the bitIdx still at the beginning of the start code
byte BitBuffer::nextStartCodeLastByte() {
    this->alignBytes();
    while (!this->isAtStartCode()) {
        this->bitIdx += 8;
    }
    buf_size_t byteI = this->byteIdx() + 3;
    return this->buffer[byteI];
}

// Just peek the bits but not actually move the bitIdx
bits BitBuffer::peek(byte count) {
    bits data = 0;
    buf_size_t BitI = this->bitIdx;
    while (count > 0) {
        byte bitIdxInByte = (byte)(BitI & 7);
        byte remainingBitNumInByte = (byte)8 - bitIdxInByte; // value between 1 ~ 8
        byte readBitNum = remainingBitNumInByte < count? remainingBitNumInByte: count;
        byte remainingAfterRead = remainingBitNumInByte - readBitNum; // remaining bit number in this byte after reading
        byte mask = ((byte)(((byte)0xff) << bitIdxInByte)) >> bitIdxInByte; //zero out the ones before bitIdxInByte, note the two byte casting is required
        data =  (data << readBitNum) | ((mask & this->buffer[BitI >> 3]) >> remainingAfterRead); // >> remainingAferRead align the read bits to the right
        count -= readBitNum;
        BitI += readBitNum;
    }

    return data;
}

// compare the next [count] bits is equals to [bs] or not.
// peek not consume
bool BitBuffer::nextBitsCompare(bits bs, byte count) {
    return bs == this->peek(count);
}

// align the bit index to the beginning of the next byte unless it's already aligned
void BitBuffer::alignBytes() {
    this->bitIdx = ((this->bitIdx + 7) >> 3) << 3;
}

//read the bits and move the bitIdx forward
bits BitBuffer::consume(byte count) {
    bits bs = this->peek(count);
    this->bitIdx += count;
    return bs;
}

bool BitBuffer::isAtStartCode() {
    buf_size_t byteI = this->byteIdx();
    return this->buffer[byteI] == 0x00 &&
            this->buffer[byteI + 1] == 0x00 &&
            this->buffer[byteI + 2] == 0x01;
}

// move the bitIdx without reading the value
void BitBuffer::skip(byte count) {
    this->bitIdx += count;
}

// calculate the index in the byte array
buf_size_t BitBuffer::byteIdx() {
    return this->bitIdx >> 3;
}

byte BitBuffer::consumeOneBit() {
    byte currentByte = this->buffer[this->byteIdx()];
    byte bitIdxInByte = this->bitIdxInCurrentByte();
    byte bit =  (currentByte >> (7 - bitIdxInByte)) & (byte)0x01;
    this->bitIdx++;
    return bit;
}

// calculate the bit index in terms of the current byte entry
byte BitBuffer::bitIdxInCurrentByte() {
    return (byte)(this->bitIdx & 7);
}

bool BitBuffer::nextStartCodeLastByteCompare(byte lastByte) {
    return this->nextStartCodeLastByte() == lastByte;
}

bool BitBuffer::isLaodSuccess() {
    return this->loadSuccess;
}
