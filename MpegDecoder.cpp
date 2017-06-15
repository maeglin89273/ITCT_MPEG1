//
// Created by maeglin89273 on 2017/6/15.
//

#include "MpegDecoder.h"

const float MpegDecoder::FPS_TABLE[] = {0, 23.976f, 24f, 25f, 29.97f, 30f, 50f, 59.94f, 60f};
const int MpegDecoder::ZIG_ZAG_ORDER_INVERSE[] = {
        0, 1, 8, 16, 9, 2, 3, 10,
        17, 24, 32, 25, 18, 11, 4, 5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13, 6, 7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63};

MpegDecoder::MpegDecoder(BitBuffer *bitBuffer) {
    this->bitBuffer = bitBuffer;
    this->intraQuantMat = new byte[] {8, 16, 19, 22, 26, 27, 29, 34,
                                        16, 16, 22, 24, 27, 29, 34, 37,
                                        19, 22, 26, 27, 29, 34, 34, 38,
                                        22, 22, 26, 27, 29, 34, 37, 40,
                                        22, 26, 27, 29, 32, 35, 40, 48,
                                        26, 27, 29, 32, 35, 40, 48, 58,
                                        26, 27, 29, 34, 38, 46, 56, 69,
                                        27, 29, 35, 38, 46, 56, 69, 83};

    this->nonIntraQuantMat = new byte[] {16, 16, 16, 16, 16, 16, 16, 16,
                                         16, 16, 16, 16, 16, 16, 16, 16,
                                        16, 16, 16, 16, 16, 16, 16, 16,
                                        16, 16, 16, 16, 16, 16, 16, 16,
                                        16, 16, 16, 16, 16, 16, 16, 16,
                                        16, 16, 16, 16, 16, 16, 16, 16,
                                        16, 16, 16, 16, 16, 16, 16, 16,
                                        16, 16, 16, 16, 16, 16, 16, 16};
}

MpegDecoder::~MpegDecoder() {
    delete [] this->intraQuantMat;
    delete [] this->nonIntraQuantMat;
}

void MpegDecoder::decode() {
    this->bitBuffer->nextStartCodeLastByte();
    do  {
        this->decodeSeqHeader();
        do {
            this->decodeGroupOfPictures();
        } while (this->bitBuffer->nextStartCodeLastByte() == StartCodeLastByte::GROUP);
    } while (this->bitBuffer->nextStartCodeLastByte() == StartCodeLastByte::SEQ_HEADER);
    bits endSeqCode = this->bitBuffer->consume(32);
    this->hexPrint(endSeqCode);
}

void MpegDecoder::hexPrint(bits data) {
    std::cout << std::hex << data << std::endl;
}

void MpegDecoder::decodeSeqHeader() {
    this->bitBuffer->skip(32); // sequence header
    this->displayWidth = this->bitBuffer->consume(12);
    this->displayHeight = this->bitBuffer->consume(12);
    this->bitBuffer->skip(4); // skip aspect ratio
    this->fps = MpegDecoder::FPS_TABLE[(int)this->bitBuffer->consume(4)];
    this->bitBuffer->skip(18); //  bit rate
    this->bitBuffer->skip(1); // marker bit
    this->bitBuffer->skip(10); // vbv buffer size
    this->bitBuffer->skip(1); // constrained parameter flag
    byte loadIntraQuantMat = this->bitBuffer->readOneBit();
    if (loadIntraQuantMat) {
        this->loadQuantMat(this->intraQuantMat);
    }

    byte loadNonIntraQuantMat = this->bitBuffer->readOneBit();
    if (loadNonIntraQuantMat) {
        this->loadQuantMat(this->nonIntraQuantMat);
    }

    this->consumeExtAndUserData();

}

void MpegDecoder::decodeGroupOfPictures() {

}

void MpegDecoder::consumeExtAndUserData() {
    byte sclByte = this->bitBuffer->nextStartCodeLastByte();
    if (sclByte == StartCodeLastByte::EXTENSION) {
        this->bitBuffer->skip(32); // extension data start code
        while (!this->bitBuffer->nextBitsCompare(1, 24)) {
            this->bitBuffer->skip(8); // extension data
        }
        sclByte = this->bitBuffer->nextStartCodeLastByte();
    }

    if (sclByte == StartCodeLastByte::USER_DATA) {
        this->bitBuffer->skip(32); // user data start code
        while (!this->bitBuffer->nextBitsCompare(1, 24)) {
            this->bitBuffer->skip(8); // user data
        }
        sclByte = this->bitBuffer->nextStartCodeLastByte();
    }
}



void MpegDecoder::loadQuantMat(byte *quantMat) {
    for (int i = 0; i < 8 * 8; i++) {
        quantMat[MpegDecoder::ZIG_ZAG_ORDER_INVERSE[i]] = (byte) this->bitBuffer->consume(8);
    }
}
