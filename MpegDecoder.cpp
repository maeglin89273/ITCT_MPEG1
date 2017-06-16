//
// Created by maeglin89273 on 2017/6/15.
//

#include "MpegDecoder.h"

const float MpegDecoder::FPS_TABLE[] = {0, 23.976f, 24.f, 25.f, 29.97f, 30.f, 50.f, 59.94f, 60.f};
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
    this->intraQuantMat = new byte[64] {8, 16, 19, 22, 26, 27, 29, 34, // default values
                                        16, 16, 22, 24, 27, 29, 34, 37,
                                        19, 22, 26, 27, 29, 34, 34, 38,
                                        22, 22, 26, 27, 29, 34, 37, 40,
                                        22, 26, 27, 29, 32, 35, 40, 48,
                                        26, 27, 29, 32, 35, 40, 48, 58,
                                        26, 27, 29, 34, 38, 46, 56, 69,
                                        27, 29, 35, 38, 46, 56, 69, 83};

    this->nonIntraQuantMat = new byte[64] {16, 16, 16, 16, 16, 16, 16, 16, // default values
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
    delete this->sequence;
}

void MpegDecoder::decode() {
    this->bitBuffer->nextStartCodeLastByte();
    do  {
        this->decodeSeqHeader();
        do {
            this->decodeGroupOfPictures();
        } while (this->bitBuffer->nextStartCodeLastByteCompare(StartCodeLastByte::GROUP));
    } while (this->bitBuffer->nextStartCodeLastByteCompare(StartCodeLastByte::SEQ_HEADER));
    bits endSeqCode = this->bitBuffer->consume(32);
    this->hexPrint(endSeqCode);
}

void MpegDecoder::hexPrint(bits data) {
    std::cout << std::hex << data << std::endl;
}

void MpegDecoder::decodeSeqHeader() {

    this->bitBuffer->skip(32); // sequence header
    int displayWidth = this->bitBuffer->consume(12);
    int displayHeight = this->bitBuffer->consume(12);
    if (this->sequence != nullptr) {
        delete this->sequence;
    }
    this->sequence = new Sequence(displayHeight, displayWidth);
    this->bitBuffer->skip(4); // skip aspect ratio
    this->fps = MpegDecoder::FPS_TABLE[(int)this->bitBuffer->consume(4)];
    this->bitBuffer->skip(18); //  bit rate
    this->bitBuffer->skip(1); // marker bit
    this->bitBuffer->skip(10); // vbv buffer size
    this->bitBuffer->skip(1); // constrained parameter flag
    byte loadIntraQuantMat = this->bitBuffer->consumeOneBit();
    if (loadIntraQuantMat) {
        this->loadQuantMat(this->intraQuantMat);
    }

    byte loadNonIntraQuantMat = this->bitBuffer->consumeOneBit();
    if (loadNonIntraQuantMat) {
        this->loadQuantMat(this->nonIntraQuantMat);
    }

    this->consumeExtAndUserData();

}

void MpegDecoder::decodeGroupOfPictures() {
    this->bitBuffer->skip(32); // group start code
    this->bitBuffer->skip(25); // time code
    this->bitBuffer->skip(1); // closed gop, assume the video is not being edited
    this->bitBuffer->skip(1); // broken link
    this->consumeExtAndUserData();
    do {
        this->decodePicture();
    } while (this->bitBuffer->nextStartCodeLastByteCompare(StartCodeLastByte::PIC));

}

void MpegDecoder::decodePicture() {
    this->bitBuffer->skip(32); // picture start code
    uint16 tmpRef = (uint16) this->bitBuffer->consume(10);
    byte picType = (byte) this->bitBuffer->consume(3);
    if (picType == Picture::PictureType::FORBIDDEN || picType == Picture::PictureType::D) {
        std::cout << "picture type error" << std::endl;
        return;
    }
    this->sequence->newPicture(picType, tmpRef);
    this->bitBuffer->skip(16); // vbv delay


    if (picType == Picture::PictureType::P || picType == Picture::PictureType::B) {
        this->consumePicForwardBackwardCode(this->sequence->picTempInfo.fullPelForwardVec, this->sequence->picTempInfo.forwradF);
    }

    if (picType == Picture::PictureType::B) {
        this->consumePicForwardBackwardCode(this->sequence->picTempInfo.fullPelBackwardVec, this->sequence->picTempInfo.backwardF);
    }

    while (this->bitBuffer->nextBitsCompare(1, 1)) {
        this->bitBuffer->skip(1); // extra bit picture = 1
        this->bitBuffer->skip(8); // extra information picture
    }
    this->bitBuffer->skip(1); // extra bit picture = 0
    this->consumeExtAndUserData();

    do {
        decodeSlice();
    } while(StartCodeLastByte::isSliceLastByte(this->bitBuffer->nextStartCodeLastByte()));

}

void MpegDecoder::decodeSlice() {
    this->sequence->sliceTempInfo.sliceVerticalPosition = (byte) (0x000000ff & this->bitBuffer->consume(32)); //slice start code
    this->sequence->mbTempInfo.quantScale = (byte) this->bitBuffer->consume(5);

    while (this->bitBuffer->nextBitsCompare(1, 1)) {
        this->bitBuffer->skip(1); // extra bit slice = 1
        this->bitBuffer->skip(8); // extra information slice
    }

    this->bitBuffer->skip(1); // extra bit slice = 1

    sliceBeginReset();

    do {
        decodeMacroblock();
    } while (!this->bitBuffer->nextBitsCompare(0, 23));

}

void MpegDecoder::sliceBeginReset() {
    this->sequence->mbTempInfo.address = (this->sequence->sliceTempInfo.sliceVerticalPosition - 1) * this->sequence->getMBWidth() - 1;
    this->sequence->mbTempInfo.pastIntraAddress = -2;
    this->sequence->blockTempInfo.dcYPast = 1024;
    this->sequence->blockTempInfo.dcCbPast = 1024;
    this->sequence->blockTempInfo.dcCrPast = 1024;
}

void MpegDecoder::decodeMacroblock() {
    while (this->bitBuffer->nextBitsCompare(0x0f, 11)) {
        this->bitBuffer->skip(11); // macroblock stuffing
    }

    int mbAddressIncrement = 0;
    while (this->bitBuffer->nextBitsCompare(0x08, 11)) {
        this->bitBuffer->skip(11); // macroblock escape
        mbAddressIncrement += 33;
    }

    mbAddressIncrement += HuffmanTable::decode(HuffmanTable::MACROBLOCK_ADDRESS_INCREMENT, this->bitBuffer);
    this->sequence->mbTempInfo.type = (byte) HuffmanTable::decode(HuffmanTable::MACROBLOCK_TYPE_LIST[this->sequence->currentPicture().getType()], this->bitBuffer);

    if (this->sequence->mbTempInfo.type & 0x10) {
        this->sequence->mbTempInfo.quantScale = (byte) this->bitBuffer->consume(5);
    }

    
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

void MpegDecoder::consumePicForwardBackwardCode(byte& fullPelVec, byte& f) {
    fullPelVec = (byte) this->bitBuffer->consume(1);
    byte fCode = (byte) this->bitBuffer->consume(3);
    byte rSize = fCode - (byte)1;
    f = ((byte)0x01) << rSize; // note, forwardFCode = [1, 7], so forwardRSize = [0, 6]
}







