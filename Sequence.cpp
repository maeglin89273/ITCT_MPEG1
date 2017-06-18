//
// Created by maeglin89273 on 6/16/17.
//

#include "Sequence.h"

Sequence::Sequence(unsigned int width, unsigned int height): mbTempInfo(this) {
    this->width = width;
    this->height = height;
    this->mbWidth = (width + 15) >> 4; // ceil to the multiple of 16 pixels
    this->mbHeight = (height + 15) >> 4;
    this->extWidth = this->mbWidth << 4;
    this->extHeight = this->mbHeight << 4;
    this->pastPic = this->futurePic = nullptr;
}

Picture& Sequence::newPicture(byte pictureType, uint16 tmpRef) {
    //TODO: manage past and future pictures here
    if (!this->picSeq.empty()) {
        Picture* potentialFuturePic = &this->currentPicture();
        if (potentialFuturePic->getType() != Picture::PictureType::B) {
            this->pastPic = this->futurePic;
            this->futurePic = potentialFuturePic;
        }
    }
    this->picSeq.push_back(Picture(this->extHeight, this->extWidth, pictureType, tmpRef));
    return this->currentPicture();
}

Picture& Sequence::currentPicture() {
    return this->picSeq.back();
}

Picture &Sequence::pastPictrue() {
    // if current picture is type P, then return the lastest non B frame not pastPic
    if (this->currentPicture().getType() == Picture::PictureType::P) {
        return *this->futurePic;
    }
    return *this->pastPic;
}

Picture &Sequence::futurePictrue() {
    return *this->futurePic;
}

unsigned int Sequence::getMBWidth() {
    return this->mbWidth;
}

unsigned int Sequence::getMBHeight() {
    return this->mbHeight;
}

