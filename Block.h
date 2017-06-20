//
// Created by maeglin89273 on 6/16/17.
//

#ifndef ITCT_MPEG1_BLOCK_H
#define ITCT_MPEG1_BLOCK_H

#include "TypeDefinition.h"

// A helper class for easy maniplulating block data, which is a one dimensional array
class Block {
private:
    byte* ptr;
    unsigned int height;
    unsigned int width;
    int trueHeight;
    int trueWidth;
    unsigned int cIdx;
    int sbWidth;
    int sX;
    int sY;
public:
    // cIdx is color index, superBlockWidth is the width of the original picture in pixels
    // upScaleX and upScaleY are for chrominance upsampling. They should be 2
    Block(byte *bufPtr, unsigned int height, unsigned int width, unsigned int cIdx, int superBlockWidth, int upScaleY=1, int upScaleX=1);
    Block(byte *bufPtr, unsigned int height, unsigned int width, unsigned int cIdx);
    ~Block();
    void set(unsigned int x, unsigned int y, byte value);
    void set(unsigned int i, byte value);
    void set(int* data);
    void set(Block &block);


    void averageBlocksSet(Block** blocks, unsigned int length);
    void add(unsigned int x, unsigned int y, int value);
    void add(unsigned int i, unsigned int value);
    void add(int* data);
    byte get(unsigned int x, unsigned int y);
    byte get(unsigned int i);
    unsigned int getWidth();
    unsigned int getHeight();

    void setBufferPtr(byte *bufPtr);
    void setCIndex(unsigned int cIdx);

    void addAverageBlocksAndHalfSet(Block **blocks, unsigned int length);

    void addAndHalfSet(unsigned int x, unsigned int y, int value);

    void addAndHalfSet(Block &block);
};


#endif //ITCT_MPEG1_BLOCK_H
