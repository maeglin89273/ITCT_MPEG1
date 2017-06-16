//
// Created by maeglin89273 on 6/16/17.
//

#include "Sequence.h"

Sequence::Sequence(int width, int height) {
    this->width = width;
    this->height = height;
    this->mbWidth = (width + 15) >> 4; // ceil to the multiple of 16 pixels
    this->mbHeight = (height + 15) >> 4;
    this->extWidth = this->mbWidth << 4;
    this->extHeight = this->mbHeight << 4;
}

Picture& Sequence::newPicture(byte pictureType, uint16 tmpRef) {
    this->picSeq.push_back(Picture(this->extHeight, this->extWidth, pictureType, tmpRef));
    return this->currentPicture();
}

Picture& Sequence::currentPicture() {
    return this->picSeq.back();
}

int Sequence::getMBWidth() {
    return this->mbWidth;
}

int Sequence::getMBHeight() {
    return this->mbHeight;
}
