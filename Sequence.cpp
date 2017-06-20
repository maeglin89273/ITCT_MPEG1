//
// Created by maeglin89273 on 6/16/17.
//

#include <queue>
#include <iostream>
#include "Sequence.h"

Sequence::Sequence(unsigned int width, unsigned int height, float fps) {
    this->width = width;
    this->height = height;
    this->fps = fps;
    this->mbWidth = (width + 15) >> 4; // ceil to the multiple of 16 pixels
    this->mbHeight = (height + 15) >> 4;
    this->extWidth = this->mbWidth << 4;
    this->extHeight = this->mbHeight << 4;
    this->pastPic = this->futurePic = this->curDispPic = nullptr;
    this->mbTempInfo.mbWidthCache = this->mbWidth;

}

Picture& Sequence::newPicture(byte pictureType, uint16 tmpRef) {
    //update the past and future picture before decoding the new picture
    if (!this->picSeq.empty()) {
        Picture* potentialFuturePic = &this->currentPicture();
        if (potentialFuturePic->getType() != Picture::Type::B) {
            this->pastPic = this->futurePic;
            this->futurePic = potentialFuturePic;
        }
    }

    // create a new picture
    this->picSeq.push_back(Picture(this->extHeight, this->extWidth, pictureType, tmpRef));

    Picture& curPic = this->currentPicture();
    if (curPic.getType() == Picture::Type::B) {
        this->curDispPic = &curPic;
    } else {
        //current pictrue is a future picture.
        // So curDispPic should be the last decoded future picture, which is a past picture in terms of the current picture
        if (this->length() > 1) {
            this->curDispPic = &this->pastPicture();
        }
    }
    return curPic;
}

Picture& Sequence::currentPicture() {
    return this->picSeq.back();
}

Picture &Sequence::pastPicture() {
    if (this->currentPicture().getType() == Picture::Type::B) {
        return *this->pastPic;
    }

    // if current picture is type I or P, then return the lastest non B picture. not pastPic
    return *this->futurePic;
}

Picture &Sequence::futurePicture() {
    return *this->futurePic;
}

Picture &Sequence::currentDisplayPicture() {
    return *this->curDispPic;
}

unsigned int Sequence::getMBWidth() {
    return this->mbWidth;
}

unsigned int Sequence::getMBHeight() {
    return this->mbHeight;
}

unsigned int Sequence::getWidth() {
    return this->width;
}

unsigned int Sequence::getHeight() {
    return this->height;
}

byte** Sequence::displaySequenceData() {

    int i = 0;
    byte** displaySeq = new byte*[this->picSeq.size()];
    Picture* past = &(this->picSeq.front());
    byte* pic;
    unsigned int picSize = 3 * this->height * this->width;
    std::list<Picture>::iterator it = this->picSeq.begin();
    it++; //skip the first picture

    //reordering
    for (; it != this->picSeq.end(); it++) {
        pic = new byte[picSize];
        if ((*it).getType() == Picture::Type::B) {
            (*it).toBGRAndCrop(pic, this->height, this->width);
            displaySeq[i++] = pic;
        } else { // when encounter the next I or P picture, then store the past I or P picture
            past->toBGRAndCrop(pic, this->height, this->width);
            displaySeq[i++] = pic;
            past = &(*it);
        }
    }
    // store the last one
    pic = new byte[picSize];
    past->toBGRAndCrop(pic, this->height, this->width);
    displaySeq[i] = pic;
    return displaySeq;
}


unsigned int Sequence::length() {
    return (unsigned int) this->picSeq.size();
}

bool Sequence::currentPictureTypeEquals(byte picType) {
    return this->currentPicture().getType() == picType;
}

bool Sequence::hasDisplayPicture() {
    return this->curDispPic != nullptr;
}

float Sequence::getFps() {
    return this->fps;
}

