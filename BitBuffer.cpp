//
// Created by maeglin89273 on 2017/6/15.
//


#include "BitBuffer.h"

BitBuffer::BitBuffer(char *fileName) {
    loadFile(fileName);
    this->bitIdx = 0;

}

void BitBuffer::loadFile(char *fileName) {
    std::ifstream jpegFile;
    jpegFile.open(fileName, std::ios::in | std::ios::binary);

    this->size = estimateFileSize(jpegFile);
    if (this->size == 0) {
        std::cout << "file " << fileName << " not found" << std::endl;
    }
    std::cout << "file size: " << this->size << " bytes" << std::endl;

    this->buffer = new byte[this->size];
    jpegFile.read((char *) this->buffer, this->size);
    jpegFile.close();

}

unsigned long BitBuffer::estimateFileSize(std::ifstream &file) {
    long begin = file.tellg();
    file.seekg(0, std::ios_base::end);
    long end = file.tellg();
    file.seekg(0, std::ios_base::beg);
    return (buf_size_t) (end - begin);
}

BitBuffer::~BitBuffer() {
    delete [] this->buffer;

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


bits BitBuffer::peek(byte count) {
    bits data = 0;
    buf_size_t BitI = this->bitIdx;
    while (count > 0) {
        byte bitIdxInByte = (byte)(BitI & 7);
        byte remainingBitNumInByte = (byte)8 - bitIdxInByte; // value between 1 ~ 8
        byte readBitNum = remainingBitNumInByte < count? remainingBitNumInByte: count;
        byte remainingAferRead = remainingBitNumInByte - readBitNum; // remaining bit number in this byte after reading
        byte mask = ((byte)0xff << bitIdxInByte) >> bitIdxInByte; //zero out the ones before bitIdxInByte
        data =  (data << readBitNum) | ((mask & this->buffer[BitI >> 3]) >> remainingAferRead); // >> remainingAferRead align the read bits to the right
        count -= readBitNum;
        BitI += readBitNum;
    }

    return data;
}

bool BitBuffer::nextBitsCompare(bits bs, byte count) {
    return bs == this->peek(count);
}

void BitBuffer::alignBytes() {
    this->bitIdx = ((this->bitIdx >> 3) << 3) + 8;
}

bits BitBuffer::consume(byte count) {
    bits bs = this->peek(count);
    this->bitIdx += count;
    return bs;
}

bool BitBuffer::isAtStartCode() {
    buf_size_t byteI = this->byteIdx();
    return this->buffer[byteI] == 0x00 &&
            this->buffer[byteI + 1] == 0x00 &&
            this->buffer[byteI + 2] == 0x00 &&
            this->buffer[byteI + 3] == 0x01;
}

void BitBuffer::skip(byte count) {
    this->bitIdx += count;

}

buf_size_t BitBuffer::byteIdx() {
    return this->bitIdx >> 3;
}

byte BitBuffer::consumeOneBit() {
    byte currentByte = this->buffer[this->byteIdx()];
    byte bitIdxInByte = this->bitIdxInCurrentByte();
    byte bit =  currentByte >> (7 - bitIdxInByte) & (byte)0x01;
    this->bitIdx++;
    return bit;
}

byte BitBuffer::bitIdxInCurrentByte() {
    return (byte)(this->byteIdx() & 7);
}

bool BitBuffer::nextStartCodeLastByteCompare(byte lastByte) {
    return this->nextStartCodeLastByte() == lastByte;
}
