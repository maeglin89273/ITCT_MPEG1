//
// Created by maeglin89273 on 2017/6/15.
//

#ifndef ITCT_MPEG1_BITBUFFER_H
#define ITCT_MPEG1_BITBUFFER_H

#include <fstream>
#include <iostream>
#include "TypeDefinition.h"

typedef unsigned long buf_size_t;
class BitBuffer {
public:
    BitBuffer(char* fileName);
    ~BitBuffer();
    byte consumeOneBit();
    byte nextStartCodeLastByte();
    bool nextStartCodeLastByteCompare(byte lastByte);
    bits peek(byte count);
    bits consume(byte count);
    void skip(byte count);
    bool nextBitsCompare(bits bs, byte count);

private:
    byte* buffer;
    buf_size_t bitIdx; //bit indexing, not byte
    buf_size_t size;

    void loadFile(char *fileName);

    buf_size_t estimateFileSize(std::ifstream& file);
    buf_size_t byteIdx(); //query the current byte index
    byte bitIdxInCurrentByte();

    bool isAtStartCode();
    void alignBytes();
};


#endif //ITCT_MPEG1_BITBUFFER_H
