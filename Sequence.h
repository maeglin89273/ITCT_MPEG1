//
// Created by maeglin89273 on 6/16/17.
//

#ifndef ITCT_MPEG1_SEQUENCE_H
#define ITCT_MPEG1_SEQUENCE_H

#include "TypeDefinition.h"
#include "Picture.h"
#include <list>

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

        bool hasMotion() {
            return this->hComp != 0 || this->vComp != 0;
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
        byte type;
        byte quant;
        byte motionForward;
        byte motionBackward;
        byte pattern;
        byte intra;
        MotionVector reconForVec;
        MotionVector preReconForVec;
        MotionVector reconBackVec;
        MotionVector preReconBackVec;
        Sequence* seqRef;
        MacroblockTemporaryInfo(Sequence* seqRef) {
            this->seqRef = seqRef;
        }
        unsigned int mbRow() {
            return ((unsigned int)this->address) / seqRef->getMBWidth();
        }
        unsigned int mbCol() {
            return ((unsigned int)this->address) % seqRef->getMBWidth();
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

    Sequence(unsigned int width, unsigned int height);
    ~Sequence();
    Picture& newPicture(byte pictureType, uint16 tmpRef);
    Picture& currentPicture();
    unsigned int getMBWidth();
    unsigned int getMBHeight();

    PictureTemporaryInfo picTempInfo;
    SliceTemporaryInfo sliceTempInfo;
    MacroblockTemporaryInfo mbTempInfo;
    BlockTemporaryInfo blockTempInfo;

    Picture &pastPictrue();
    Picture &futurePictrue();

    void toDisplay();
    unsigned int length();

    Picture **getPictureArray();

private:
    unsigned int width;
    unsigned int height;

    unsigned int mbWidth;
    unsigned int mbHeight;

    unsigned int extWidth;
    unsigned int extHeight;
    std::list<Picture> picSeq;
    Picture** displaySeq;
    Picture *pastPic;
    Picture *futurePic;


};


#endif //ITCT_MPEG1_SEQUENCE_H
