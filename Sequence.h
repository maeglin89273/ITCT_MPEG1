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
        int mbRow() {
            return this->address / seqRef->getMBWidth();
        }
        int mbCol() {
            return this->address % seqRef->getMBWidth();
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

    Sequence(int width, int height);
    Picture& newPicture(byte pictureType, uint16 tmpRef);
    Picture& currentPicture();
    int getMBWidth();
    int getMBHeight();

    PictureTemporaryInfo picTempInfo;
    SliceTemporaryInfo sliceTempInfo;
    MacroblockTemporaryInfo mbTempInfo;
    BlockTemporaryInfo blockTempInfo;

private:
    int width;
    int height;

    int mbWidth;
    int mbHeight;

    int extWidth;
    int extHeight;
    std::list<Picture> picSeq;

};


#endif //ITCT_MPEG1_SEQUENCE_H
