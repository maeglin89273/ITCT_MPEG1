//
// Created by maeglin89273 on 6/16/17.
//

#ifndef ITCT_MPEG1_PICTURE_H
#define ITCT_MPEG1_PICTURE_H

#include "TypeDefinition.h"
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

    Picture(int height, int width, byte type, uint16 tmpRef);
    Picture(const Picture& pic);
    int getWidth();
    int getHeight();
    unsigned char* getData();
    byte getType();
    uint16 getTemporalReference();
    void toBGRAndCrop(int height, int width);
    static byte clamp(float value);
    ~Picture();

private:
    byte* data;
    int width;
    int height;
    byte type;
    uint16 tmpRef;
    ColorMode colorMode;
};

#endif //ITCT_MPEG1_PICTURE_H
