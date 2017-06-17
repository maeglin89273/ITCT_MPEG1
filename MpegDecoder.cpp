//
// Created by maeglin89273 on 2017/6/15.
//

#include "MpegDecoder.h"
#include "Sequence.h"

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
    delete this->seq;
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

    this->bitBuffer->skip(32); // seq header
    int displayWidth = this->bitBuffer->consume(12);
    int displayHeight = this->bitBuffer->consume(12);
    if (this->seq != nullptr) {
        delete this->seq;
    }
    this->seq = new Sequence(displayHeight, displayWidth);
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
    this->seq->newPicture(picType, tmpRef);
    this->bitBuffer->skip(16); // vbv delay


    if (picType == Picture::PictureType::P || picType == Picture::PictureType::B) {
        this->consumePicForwardBackwardCode(this->seq->picTempInfo.fullPelForwardVec, this->seq->picTempInfo.forwardRSize, this->seq->picTempInfo.forwradF);
    }

    if (picType == Picture::PictureType::B) {
        this->consumePicForwardBackwardCode(this->seq->picTempInfo.fullPelBackwardVec, this->seq->picTempInfo.backwardRSize, this->seq->picTempInfo.backwardF);
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
    this->seq->sliceTempInfo.sliceVerticalPosition = (byte) (0x000000ff & this->bitBuffer->consume(32)); //slice start code
    this->seq->mbTempInfo.quantScale = (byte) this->bitBuffer->consume(5);

    while (this->bitBuffer->nextBitsCompare(1, 1)) {
        this->bitBuffer->skip(1); // extra bit slice = 1
        this->bitBuffer->skip(8); // extra information slice
    }

    this->bitBuffer->skip(1); // extra bit slice = 1

    sliceBeginReset();
    bool beginMB = true;
    do {
        decodeMacroblock(beginMB);
        beginMB = false;
    } while (!this->bitBuffer->nextBitsCompare(0, 23));

}

void MpegDecoder::sliceBeginReset() {
    this->seq->mbTempInfo.address = (this->seq->sliceTempInfo.sliceVerticalPosition - 1) * this->seq->getMBWidth() - 1;
    this->seq->mbTempInfo.pastIntraAddress = -2;
    // according to the standard 2.4.4.1
    this->seq->blockTempInfo.resetDcPast();
    // according to the standard 2.4.4.2
    this->seq->mbTempInfo.reconForVec.resetToZero();
    this->seq->mbTempInfo.preReconForVec.resetToZero();

}

void MpegDecoder::decodeMacroblock(bool beginMB) {
    while (this->bitBuffer->nextBitsCompare(0x0f, 11)) {
        this->bitBuffer->skip(11); // macroblock stuffing
    }

    int mbAddressIncrement = 0;
    while (this->bitBuffer->nextBitsCompare(0x08, 11)) {
        this->bitBuffer->skip(11); // macroblock escape
        mbAddressIncrement += 33;
    }

    mbAddressIncrement += HuffmanTable::decode(HuffmanTable::MACROBLOCK_ADDRESS_INCREMENT, this->bitBuffer);


    this->seq->mbTempInfo.address += mbAddressIncrement;

    // when encountering skipped macroblock
    if (mbAddressIncrement > 1) {
        this->skippedMacroblockReset();
    }

    byte type = (byte) HuffmanTable::decode(HuffmanTable::MACROBLOCK_TYPE_LIST[this->seq->currentPicture().getType()],
                                            this->bitBuffer);
    this->decodeMacroblockType(type);
    if (this->seq->mbTempInfo.quant) {
        this->seq->mbTempInfo.quantScale = (byte) this->bitBuffer->consume(5);
    }

    byte mbIntra = this->seq->mbTempInfo.intra;
    // note, if macroblock is intra, there is no motion information. This can be verified in type VLC table
    //so we skip checking motionForward and motionBackward flags if the macroblock is intra
    if (mbIntra) {
        //no motion inforamtion, according to the standard 2.4.4.2, reset reconstructed motion vectors to zeros
        this->seq->mbTempInfo.reconForVec.resetToZero();
        this->seq->mbTempInfo.preReconForVec.resetToZero();

    } else {
        // non intra-coded macroblock, reset dct dc past, according to the standard 2.4.4.1
        this->seq->blockTempInfo.resetDcPast();

        Sequence::PictureTemporaryInfo& picTempInfo = this->seq->picTempInfo;
        if (this->seq->mbTempInfo.motionForward) {
            // forward H
            this->decodeReconMotionVec(picTempInfo.forwradF, picTempInfo.forwardRSize, picTempInfo.fullPelForwardVec,
                                       this->seq->mbTempInfo.reconForVec.hComp,
                                       this->seq->mbTempInfo.preReconForVec.hComp);
            // forward V
            this->decodeReconMotionVec(picTempInfo.forwradF, picTempInfo.forwardRSize, picTempInfo.fullPelForwardVec,
                                       this->seq->mbTempInfo.reconForVec.vComp,
                                       this->seq->mbTempInfo.preReconForVec.vComp);
        } else if (this->seq->currentPicture().getType() == Picture::PictureType::P) {
            // no motion information in p frame, reset motion vectors to 0
            this->seq->mbTempInfo.reconForVec.resetToZero();
            this->seq->mbTempInfo.preReconForVec.resetToZero();
        }

        if (this->seq->mbTempInfo.motionBackward) {
            // backward H
            this->decodeReconMotionVec(picTempInfo.backwardF, picTempInfo.backwardRSize, picTempInfo.fullPelBackwardVec,
                                       this->seq->mbTempInfo.reconForVec.hComp,
                                       this->seq->mbTempInfo.preReconForVec.hComp);
            // backward V
            this->decodeReconMotionVec(picTempInfo.backwardF, picTempInfo.backwardRSize, picTempInfo.fullPelBackwardVec,
                                       this->seq->mbTempInfo.reconBackVec.vComp,
                                       this->seq->mbTempInfo.preReconBackVec.vComp);
        }
    }
    byte cbp = 0;
    if (this->seq->mbTempInfo.pattern) {
        cbp = (byte) HuffmanTable::decode(HuffmanTable::CODE_BLOCK_PATTERN, this->bitBuffer);
    }
    if (mbIntra) {
        cbp = 0x3f; // binary 00111111
    }

    byte mask = 0x20; // use mask to select the bit in cbp to evaluate, from left to right, 6 bits
    for (int i = 0; i < 6; i++) {
        if (cbp & mask) {
            decodeBlock(i, mbIntra);
        }
        mask >>= 1;
    }

    //impossible being D frame
//
//    if (this->seq->currentPicture().getType() == Picture::PictureType::D) {
//        this->bitBuffer->skip(1); // end of macroblock
//    }
}

void MpegDecoder::decodeReconMotionVec(byte f, byte rSize, byte fullPelVec, int &reconVecComp, int &prevReconVecComp) {
    int moCode = HuffmanTable::decode(HuffmanTable::MOTION, this->bitBuffer);

    int complR = 0;
    if ((f != 1) && (moCode != 0)) {
        bits moHForwardR = bitBuffer->consume(rSize);
        complR = f - 1 - (int)moHForwardR;
    }
    int little = moCode * f;
    int big = 0;
    if (little != 0) {
        if (little > 0) {
            little -= complR;
            big = little - f * 32;
        } else {
            little += complR;
            big = little + f * 32;
        }
    }

    int max = 16 * f - 1;
    int min = -16 * f;
    int newVecComp = prevReconVecComp + little;
    if (newVecComp <= max && newVecComp >= min) {
        reconVecComp = newVecComp;
    } else {
        reconVecComp = prevReconVecComp + big;
    }
    prevReconVecComp = reconVecComp;
    if (fullPelVec) {
        reconVecComp <<= 1;
    }
}

void MpegDecoder::skippedMacroblockReset() {

    // reset dct dc past, according to the standard 2.4.4.1
    this->seq->blockTempInfo.resetDcPast();

    if (this->seq->currentPicture().getType() == Picture::PictureType::P) {
        // set reconstructed motion vectors to zero
        this->seq->mbTempInfo.reconForVec.resetToZero();
        this->seq->mbTempInfo.preReconForVec.resetToZero();
    }

    if (this->seq->currentPicture().getType() == Picture::PictureType::B) {
        // reset reconstructed motion vectors
    }
}

void MpegDecoder::decodeBlock(int i, byte mbIntra) {
    if (mbIntra) {
        const int* sizeTable = i < 4? HuffmanTable::DC_SIZE_LUMINANCE: HuffmanTable::DC_SIZE_CHROMINANCE;
        byte dcSize = (byte) HuffmanTable::decode(sizeTable, this->bitBuffer);
        if (dcSize) {
            byte dcDiff = (byte) bitBuffer->consume(dcSize);
        }
    } else {
        bits coeffFirst = (bits) HuffmanTable::decode(HuffmanTable::DCT_COEFF, this->bitBuffer);

    }

    // skip checking picture type is not D type
    while (!this->bitBuffer->nextBitsCompare(2, 2)) {
        bits coeffNext = (bits) HuffmanTable::decode(HuffmanTable::DCT_COEFF, this->bitBuffer);
    }

    this->bitBuffer->skip(2); // end of block = 2
}


void MpegDecoder::decodeMacroblockType(byte type) {
    this->seq->mbTempInfo.quant = (byte) (type & 0x10);
    this->seq->mbTempInfo.motionForward = (byte) (type & 0x08);
    this->seq->mbTempInfo.motionBackward = (byte) (type & 0x04);
    this->seq->mbTempInfo.pattern = (byte) (type & 0x02);
    this->seq->mbTempInfo.intra = (byte) (type & 0x01);
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

void MpegDecoder::consumePicForwardBackwardCode(byte& fullPelVec, byte& rSize, byte& f) {
    fullPelVec = (byte) this->bitBuffer->consume(1);
    byte fCode = (byte) this->bitBuffer->consume(3);
    rSize = fCode - (byte)1;
    f = ((byte)0x01) << rSize; // note, forwardFCode = [1, 7], so forwardRSize = [0, 6]
}













