//
// Created by maeglin89273 on 6/16/17.
//

#include <cmath>
#include <cstring>
#include "Picture.h"
#include "Block.h"

Picture::Picture(int height, int width, byte type, uint16 tmpRef) {
    this->width = width;
    this->height = height;
    this->type = type;
    this->tmpRef = tmpRef;
    this->data = new byte[width * height * 3];
    this->colorMode = ColorMode::Y_Cb_Cr;
}

Picture::Picture(const Picture &pic) {
    this->width = pic.width;
    this->height = pic.height;
    this->type = pic.type;
    this->tmpRef = tmpRef;
    unsigned int size = (unsigned int) (this->width * this->height * 3);
    this->data = new byte[size];
    memcpy(this->data, pic.data, size);
    this->colorMode = pic.colorMode;
}

Picture::~Picture() {
    delete [] this->data;
}

unsigned char* Picture::getData() {
    return this->data;
}

int Picture::getHeight() {
    return this->height;
}

int Picture::getWidth() {
    return this->width;
}

void Picture::toBGRAndCrop(int height, int width) {
    if (this->colorMode == ColorMode::BGR) {
        return;
    }

    Block *inputY = new Block(this->data, this->height, this->width, 0);
    Block *inputCb = new Block(this->data, this->height, this->width, 1);
    Block *inputCr = new Block(this->data, this->height, this->width, 2);

    byte* bgrData = new byte[width * height * 3];
    Block *outputB = new Block(bgrData, height, width, 0);
    Block *outputG = new Block(bgrData, height, width, 1);
    Block *outputR = new Block(bgrData, height, width, 2);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
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

    delete [] this->data;
    this->data = bgrData;
    this->width = width;
    this->height = height;
    this->colorMode = ColorMode::BGR;

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

byte Picture::getType() {
    return this->type;
}

uint16 Picture::getTemporalReference() {
    return this->tmpRef;
}



