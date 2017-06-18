//
// Created by maeglin89273 on 2017/6/15.
//

#ifndef ITCT_MPEG1_MPEGDECODER_H
#define ITCT_MPEG1_MPEGDECODER_H


#include "BitBuffer.h"
#include "Sequence.h"
#include "HuffmanTable.h"
#include "FIDCT.h"
#include "Block.h"

class MpegDecoder {
public:
    MpegDecoder(BitBuffer* bitBuffer);
    ~MpegDecoder();
    void decode();

private:
    BitBuffer* bitBuffer;
    Sequence* seq;
    float fps;
    byte* intraQuantMat;
    byte* nonIntraQuantMat;
    void hexPrint(bits data);
    int *dctZZ;
    FIDCT fidct;

    class StartCodeLastByte {
    public:
        static const byte PIC = 0x00;
        static const byte SEQ_HEADER = 0xb3;
        static const byte USER_DATA = 0xb2;
        static const byte EXTENSION = 0xb5;
        static const byte SEQ_END = 0xb7;
        static const byte GROUP = 0xb8;
        static const byte SLICE_FIRST = 0x01;
        static const byte SLICE_LAST = 0xaf;
        static bits fullStartCode(byte lastByte) {
            return (bits) (0x0100 | lastByte);
        }
        static bool isSliceLastByte(byte lastByte) {
            return lastByte >= SLICE_FIRST && lastByte <= SLICE_LAST;
        }
    };

    static const float FPS_TABLE[];
    static const int ZIG_ZAG_ORDER[64];
    static const int ZIG_ZAG_ORDER_INVERSE[64];

    void decodeSeqHeader();

    void decodeGroupOfPictures();

    void consumePicForwardBackwardCode(byte& fullPelVec, byte& rSize, byte& f);

    void consumeExtAndUserData();

    void loadQuantMat(byte *quantMat);

    void decodePicture();

    void decodeSlice();

    void decodeMacroblock(bool firstMB);

    void sliceBeginReset();

    void decodeMacroblockType(byte type);

    void decodeBlock(int blockI, byte mbIntra);

    void skippedMacroblockReset();

    void decodeReconMotionVec(byte f, byte rSize, byte fullPelVec, int &reconVecComp, int &prevReconVecComp);


    void setZeros(int *array, int length);

    void decodeRunLenCoeff(uint16 coeff, int& run, int& level);

    int sign(int val);

    void reconIntraDctCoef(int blockI, int *dctRecon, int *dctZZ, int &dcPast);

    void doIDCT(int *coeffs);

    void reconNonIntraDctCoef(int *dctRecon, int *dctZZ);

    void fillSkippedMacroblocks(int increment);
};


#endif //ITCT_MPEG1_MPEGDECODER_H
