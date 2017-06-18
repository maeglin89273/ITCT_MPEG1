//
// Created by maeglin89273 on 6/16/17.
//

#ifndef ITCT_MPEG1_PICTURE_H
#define ITCT_MPEG1_PICTURE_H

#include "TypeDefinition.h"
#include "Block.h"

class Picture {


public:
    enum ColorMode {
        Y_Cb_Cr, BGR
    };
    class PictureType {
    public:
        const static byte FORBIDDEN = 0 ;
        const static byte I = 1;
        const static byte P = 2;
        const static byte B = 3;
        const static byte D = 4;
    };

    Picture(unsigned int height, unsigned int width, byte type, uint16 tmpRef);
    Picture(const Picture& pic);
    Picture& operator=(const Picture& pic);
    unsigned int getWidth();
    unsigned int getHeight();
    unsigned char* getData();
    byte getType();
    uint16 getTemporalReference();
    void toBGRAndCrop(unsigned int height, unsigned int width);
    Block* getBlock(unsigned int mbCol, unsigned int mbRow, int blockI);
    Block* getMacroblock(unsigned int mbCol, unsigned int mbRow, int cIdx);
    Block* getMacroblock(unsigned int mbCol, unsigned int mbRow, int cIdx, int pelOffsetX, int pelOffsetY);
    static byte clamp(float value);
    static byte clamp(int value);
    ~Picture();

private:
    byte* data;
    unsigned int width;
    unsigned int height;
    byte type;
    uint16 tmpRef;
    ColorMode colorMode;

    unsigned int computeMBPelIdx(unsigned int mbCol, unsigned int mbRow);
};

#endif //ITCT_MPEG1_PICTURE_H
