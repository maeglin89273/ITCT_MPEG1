//
// Created by maeglin89273 on 6/16/17.
//

#ifndef ITCT_MPEG1_BLOCK_H
#define ITCT_MPEG1_BLOCK_H

#include "TypeDefinition.h"

class Block {
private:
    byte* ptr;
    int height;
    int width;
    int trueHeight;
    int trueWidth;
    int cIdx;
    int sbWidth;
    int sX;
    int sY;
public:
    Block(byte *bufPtr, int height, int width, int cIdx, int superBlockWidth, int upScaleY=1, int upSscaleX=1);
    Block(byte *bufPtr, int height, int width, int cIdx);
    ~Block();
    void set(int x, int y, byte value);
    void set(int i, byte value);
    void set(int* data);
    void add(int x, int y, int value);
    void add(int i, int value);
    void add(int* data);
    byte get(int x, int y);
    byte get(int i);
    int getWidth();
    int getHeight();

    void setBufferPtr(byte *bufPtr);

    void setCIndex(int cIdx);
};


#endif //ITCT_MPEG1_BLOCK_H
