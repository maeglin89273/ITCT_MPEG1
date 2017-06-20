//
// Created by maeglin89273 on 6/16/17.
//

#ifndef ITCT_MPEG1_SEQUENCE_H
#define ITCT_MPEG1_SEQUENCE_H

#include "TypeDefinition.h"
#include "Picture.h"
#include <list>

//The class stores and manages decoded data, and the picture order
class Sequence {
public:
    class MotionVector {
    public:
        int hComp;
        int vComp;
        MotionVector() {
            this->hComp = 0;
            this->vComp = 0;
        }

        MotionVector(const MotionVector& vec) {
            this->hComp = vec.hComp;
            this->vComp = vec.vComp;
        }
        MotionVector& operator=(const MotionVector& vec) {
            this->hComp = vec.hComp;
            this->vComp = vec.vComp;
            return *this;
        }

        void resetToZero() {
            this->hComp = 0;
            this->vComp = 0;
        }


    };
    class PictureTemporaryInfo {
    public:
        byte forwradF;
        byte forwardRSize;
        byte fullPelForwardVec;
        byte backwardF;
        byte backwardRSize;
        byte fullPelBackwardVec;

    };

    class SliceTemporaryInfo {
    public:
        byte sliceVerticalPosition;

        bool isFirstSlice() {
            return sliceVerticalPosition == 1;
        }

    };

    class MacroblockTemporaryInfo {
    public:
        int address;
        int pastIntraAddress;
        byte quantScale;
        byte quant;
        byte motionForward;
        byte motionBackward;
        byte pattern;
        byte intra;
        MotionVector reconForVec;
        MotionVector preReconForVec;
        MotionVector reconBackVec;
        MotionVector preReconBackVec;

        unsigned int mbWidthCache;
        unsigned int mbRow() {
            return ((unsigned int)this->address) / this->mbWidthCache;
        }
        unsigned int mbCol() {
            return ((unsigned int)this->address) % this->mbWidthCache;
        }


    };

    class BlockTemporaryInfo {
    public:
        int dcYPast;
        int dcCbPast;
        int dcCrPast;

        void resetDcPast() {
            this->dcYPast = 1024;
            this->dcCbPast = 1024;
            this->dcCrPast = 1024;
        }

    };

    Sequence(unsigned int width, unsigned int height, float fps);

    Picture& newPicture(byte pictureType, uint16 tmpRef);
    bool hasDisplayPicture();
    bool currentPictureTypeEquals(byte picType);

    unsigned int getWidth();
    unsigned int getHeight();
    unsigned int getMBWidth();
    unsigned int getMBHeight();

    PictureTemporaryInfo picTempInfo;
    SliceTemporaryInfo sliceTempInfo;
    MacroblockTemporaryInfo mbTempInfo;
    BlockTemporaryInfo blockTempInfo;

    Picture& currentPicture();
    Picture &pastPicture();
    Picture &futurePicture();
    Picture& currentDisplayPicture();

    byte** displaySequenceData();
    unsigned int length();

    float getFps();

private:
    unsigned int width;
    unsigned int height;

    unsigned int mbWidth;
    unsigned int mbHeight;

    unsigned int extWidth;
    unsigned int extHeight;
    float fps;
    std::list<Picture> picSeq;
    Picture *pastPic;
    Picture *futurePic;
    Picture *curDispPic;

};


#endif //ITCT_MPEG1_SEQUENCE_H
