//
// Created by maeglin89273 on 6/16/17.
//

#include "Block.h"
#include "Picture.h"

Block::Block(byte *bufPtr, unsigned int height, unsigned int width, unsigned int cIdx, int superBlockWidth, int upScaleY, int upScaleX) {
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

Block::Block(byte *bufPtr, unsigned int height, unsigned int width, unsigned int cIdx):
Block(bufPtr, height, width, cIdx, width, 1, 1) {

}

Block::~Block() {
    this->ptr = nullptr;
}

void Block::set(unsigned int x, unsigned int y, byte value) {
    for (unsigned int ty = sY * y; ty < sY * y + sY; ty++) { // we need to consider the upsampling scale
        for (unsigned int tx = sX * x; tx < sX * x + sX; tx++) {
            this->ptr[tx * 3 + ty * this->sbWidth] = value;
        }
    }
}

byte Block::get(unsigned int x, unsigned int y) {
    x = sX * x;
    y = sY * y;
    return this->ptr[x * 3 + y * this->sbWidth];
}

unsigned int Block::getWidth() {
    return this->width;
}

unsigned int Block::getHeight() {
    return this->height;
}

void Block::set(unsigned int i, byte value) {
    unsigned int y = i / this->width;
    unsigned int x = i % this->width;
    this->set(x, y, value);
}

byte Block::get(unsigned int i) {
    unsigned int y = i / this->width;
    unsigned int x = i % this->width;
    return get(x, y);
}

void Block::setBufferPtr(byte *bufPtr) {
    this->ptr = bufPtr + this->cIdx;
}

void Block::setCIndex(unsigned int cIdx) {
    this->ptr = this->ptr - this->cIdx + cIdx;
    this->cIdx = cIdx;
}

void Block::add(unsigned int x, unsigned int y, int value) {
    for (unsigned int ty = sY * y; ty < sY * y + sY; ty++) {
        for (unsigned int tx = sX * x; tx < sX * x + sX; tx++) {
            this->ptr[tx * 3 + ty * this->sbWidth] = Picture::clamp(this->ptr[tx * 3 + ty * this->sbWidth] + value);
        }
    }
}

void Block::add(unsigned int i, unsigned int value) {
    unsigned int y = i / this->width;
    unsigned int x = i % this->width;
    this->add(x, y, value);
}

// this method is designed to do forward and backword motion vectors averaging
void Block::addAndHalfSet(unsigned int x, unsigned int y, int value) {
    for (unsigned int ty = sY * y; ty < sY * y + sY; ty++) {
        for (unsigned int tx = sX * x; tx < sX * x + sX; tx++) {
            this->ptr[tx * 3 + ty * this->sbWidth] = Picture::clamp((this->ptr[tx * 3 + ty * this->sbWidth] + value) / 2.0f);
        }
    }
}

void Block::set(int *data) {
    unsigned int i = 0;
    for (unsigned int y = 0; y < this->height; y++) {
        for (unsigned int x = 0; x < this->width; x++) {
            this->set(x, y, Picture::clamp(data[i++]));
        }
    }
}


void Block::add(int *data) {
    unsigned int i = 0;
    for (unsigned int y = 0; y < this->height; y++) {
        for (unsigned int x = 0; x < this->width; x++) {
            this->add(x, y, data[i++]);
        }
    }
}

// Average the block data and then set. It's for macroblock referencing
void Block::averageBlocksSet(Block **blocks, unsigned int length) {
    for (unsigned int y = 0; y < this->height; y++) {
        for (unsigned int x = 0; x < this->width; x++) {
            int sum = 0;
            for (unsigned int i = 0; i < length; i++) {
                sum += blocks[i]->get(x, y);
            }
            this->set(x, y, Picture::clamp(sum / (float) length));
        }
    }

}

void Block::addAverageBlocksAndHalfSet(Block **blocks, unsigned int length) {
    for (unsigned int y = 0; y < this->height; y++) {
        for (unsigned int x = 0; x < this->width; x++) {
            int sum = 0;
            for (unsigned int i = 0; i < length; i++) {
                sum += blocks[i]->get(x, y);
            }
            this->addAndHalfSet(x, y, Picture::clamp(sum / (float) length));
        }
    }
}


void Block::set(Block &block) {
    for (unsigned int y = 0; y < this->height; y++) {
        for (unsigned int x = 0; x < this->width; x++) {
            this->set(x, y, block.get(x, y));
        }
    }
}

void Block::addAndHalfSet(Block &block) {
    for (unsigned int y = 0; y < this->height; y++) {
        for (unsigned int x = 0; x < this->width; x++) {
            this->addAndHalfSet(x, y, block.get(x, y));
        }
    }
}





