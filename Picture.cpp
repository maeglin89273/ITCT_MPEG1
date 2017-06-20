//
// Created by maeglin89273 on 6/16/17.
//

#include <cmath>
#include <cstring>
#include "Picture.h"
#include "Block.h"

Picture::Picture(unsigned int height, unsigned int width, byte type, uint16 tmpRef) {
    this->width = width;
    this->height = height;
    this->type = type;
    this->tmpRef = tmpRef;
    this->data = new byte[width * height * 3];
}
// implement the copying operation in case of the usage in the c++ list data structure
Picture::Picture(const Picture &pic) {
    this->width = pic.width;
    this->height = pic.height;
    this->type = pic.type;
    this->tmpRef = tmpRef;
    unsigned int size = this->width * this->height * 3;
    this->data = new byte[size];
    memcpy(this->data, pic.data, size);
}

// implement the copying operation in case of the usage in the c++ list data structure
Picture &Picture::operator=(const Picture &pic) {
    this->width = pic.width;
    this->height = pic.height;
    this->type = pic.type;
    this->tmpRef = tmpRef;
    unsigned int size = this->width * this->height * 3;
    delete [] this->data;
    this->data = new byte[size];
    memcpy(this->data, pic.data, size);
    return *this;
}

Picture::~Picture() {
    delete [] this->data;
}

unsigned char* Picture::getData() {
    return this->data;
}

unsigned int Picture::getHeight() {
    return this->height;
}

unsigned int Picture::getWidth() {
    return this->width;
}

void Picture::toBGRAndCrop(byte* output, unsigned int cropHeight, unsigned int cropWidth) {
    //create 3 input and output blocks for easy copying and converting color channels
    Block *inputY = new Block(this->data, this->height, this->width, 0);
    Block *inputCb = new Block(this->data, this->height, this->width, 1);
    Block *inputCr = new Block(this->data, this->height, this->width, 2);


    Block *outputB = new Block(output, cropHeight, cropWidth, 0);
    Block *outputG = new Block(output, cropHeight, cropWidth, 1);
    Block *outputR = new Block(output, cropHeight, cropWidth, 2);
    for (unsigned int y = 0; y < cropHeight; y++) {
        for (unsigned int x = 0; x < cropWidth; x++) {
            float yc = inputY->get(x, y);
            float cb = inputCb->get(x, y);
            float cr = inputCr->get(x, y);
            outputB->set(x, y, clamp(yc + 1.772f * (cb - 128)));
            outputG->set(x, y, clamp(yc - 0.34414f * (cb - 128) - 0.71414f * (cr - 128)));
            outputR->set(x, y, clamp(yc + 1.402f * (cr - 128)));
        }
    }

    delete inputY;
    delete inputCb;
    delete inputCr;
    delete outputB;
    delete outputG;
    delete outputR;

}

byte Picture::clamp(float value) {
    if (value > 255) {
        return 255;
    }
    if (value < 0) {
        return 0;
    }
    return (byte)(round(value));
}

byte Picture::clamp(int value) {
    if (value > 255) {
        return 255;
    }
    if (value < 0) {
        return 0;
    }
    return (byte)(value);
}


byte Picture::getType() {
    return this->type;
}

// it's useless, actually
uint16 Picture::getTemporalReference() {
    return this->tmpRef;
}

// get the block at the given macroblock coordinate
Block *Picture::getBlock(unsigned int mbCol, unsigned int mbRow, int blockI) {

    unsigned int pelIdx = this->computeMBPelIdx(mbCol, mbRow);

    //Y block
    if (blockI < 4) {
        if (blockI % 2 == 1) { // block 1, 3
            pelIdx += 8 * 3;
        }

        if (blockI > 1) { // block 2, 3
            pelIdx += (this->width * 3) * 8;
        }
        return new Block(&(this->data[pelIdx]), 8, 8, 0, this->width);
    }
    //Cb, Cr block
    return new Block(&(this->data[pelIdx]), 8, 8, blockI == 4? 1: 2, this->width, 2, 2);
}

Block *Picture::getMacroblock(unsigned int mbCol, unsigned int mbRow, unsigned int cIdx) {
    unsigned int pelIdx = computeMBPelIdx(mbCol, mbRow);
    if (cIdx == 0) {
        return new Block(&(this->data[pelIdx]), 16, 16, cIdx, this->width);
    }
    //Cb, Cr macroblock
    return new Block(&(this->data[pelIdx]), 8, 8, cIdx, this->width, 2, 2);
}

Block *Picture::getMacroblock(unsigned int mbCol, unsigned int mbRow, unsigned int cIdx, int pelOffsetX, int pelOffsetY) {
    unsigned int pelIdx = computeMBPelIdx(mbCol, mbRow);
    pelIdx += pelOffsetX * 3;
    pelIdx += (this->width * 3) * pelOffsetY;

    if (cIdx == 0) {
        return new Block(&(this->data[pelIdx]), 16, 16, cIdx, this->width);
    }
    //Cb, Cr macroblock
    return new Block(&(this->data[pelIdx]), 8, 8, cIdx, this->width, 2, 2);
}

//convert the macroblock coordinate to the corresponding byte array index
unsigned int Picture::computeMBPelIdx(unsigned int mbCol, unsigned int mbRow) {
    return (this->width * 3) * (16 * mbRow) + 3 * (16 * mbCol); // y offset + x offset
}







