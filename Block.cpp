//
// Created by maeglin89273 on 6/16/17.
//

#include "Block.h"

Block::Block(byte *bufPtr, int height, int width, int cIdx, int superBlockWidth, int upScaleY, int upScaleX) {
    this->ptr = bufPtr + cIdx;
    this->cIdx = cIdx;
    this->width = width;
    this->height = height;
    this->sbWidth = 3 * superBlockWidth;
    this->sX = upScaleX;
    this->sY = upScaleY;
    this->trueHeight = upScaleY * this->height;
    this->trueWidth = upScaleX * this->width;
}

Block::Block(byte *bufPtr, int height, int width, int cIdx):
Block(bufPtr, height, width, cIdx, width, 1, 1) {

}

Block::~Block() {
    this->ptr = nullptr;
}

void Block::set(int x, int y, byte value) {

    for (int ty = sY * y; ty < sY * y + sY; ty++) {
        for (int tx = sX * x; tx < sX * x + sX; tx++) {
            this->ptr[tx * 3 + ty * this->sbWidth] = value;
        }
    }
}

byte Block::get(int x, int y) {
    x = sX * x;
    y = sY * y;
    return this->ptr[x * 3 + y * this->sbWidth];
}

int Block::getWidth() {
    return this->width;
}

int Block::getHeight() {
    return this->height;
}

void Block::set(int i, byte value) {
    int y = i / this->width;
    int x = i % this->width;
    this->set(x, y, value);
}

byte Block::get(int i) {
    int y = i / this->width;
    int x = i % this->width;
    return get(x, y);
}

void Block::setBufferPtr(byte *bufPtr) {
    this->ptr = bufPtr + this->cIdx;
}

void Block::setCIndex(int cIdx) {
    this->ptr = this->ptr - this->cIdx + cIdx;
    this->cIdx = cIdx;
}


