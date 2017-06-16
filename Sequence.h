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
    class PictureTemporaryInfo {
    public:
        byte forwradF;
        byte fullPelForwardVec;
        byte backwardF;
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
    };

    class BlockTemporaryInfo {
    public:
        int dcYPast;
        int dcCbPast;
        int dcCrPast;

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
