//
// Created by maeglin89273 on 2017/6/15.
//

#include <cstring>
#include "MpegDecoder.h"

const float MpegDecoder::FPS_TABLE[] = {0, 23.976f, 24.f, 25.f, 29.97f, 30.f, 50.f, 59.94f, 60.f};
const int MpegDecoder::ZIG_ZAG_ORDER[] = {
        0,   1,   5,  6,   14,  15,  27,  28,
        2,   4,   7,  13,  16,  26,  29,  42,
        3,   8,  12,  17,  25,  30,  41,  43,
        9,   11, 18,  24,  31,  40,  44,  53,
        10,  19, 23,  32,  39,  45,  52,  54,
        20,  22, 33,  38,  46,  51,  55,  60,
        21,  34, 37,  47,  50,  56,  59,  61,
        35,  36, 48,  49,  57,  58,  62,  63 };

const int MpegDecoder::ZIG_ZAG_ORDER_INVERSE[] = {
        0, 1, 8, 16, 9, 2, 3, 10,
        17, 24, 32, 25, 18, 11, 4, 5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13, 6, 7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63};



MpegDecoder::MpegDecoder(BitBuffer *bitBuffer, Renderer* renderer) {
    this->bitBuffer = bitBuffer;
    this->renderer = renderer;
    this->seq = nullptr;
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
    this->dctZZ = new int[64];

}

MpegDecoder::~MpegDecoder() {
    delete[] this->intraQuantMat;
    delete[] this->nonIntraQuantMat;
    delete[] this->dctZZ;
    if (this->seq != nullptr) {
        delete this->seq;
    }
}

Sequence * MpegDecoder::decode() {
    //follow the standard structure
    this->bitBuffer->nextStartCodeLastByte();
    do  {
        this->decodeSeqHeader();
        do {
            this->decodeGroupOfPictures();
        } while (this->bitBuffer->nextStartCodeLastByteCompare(StartCodeLastByte::GROUP));
    } while (this->bitBuffer->nextStartCodeLastByteCompare(StartCodeLastByte::SEQ_HEADER));

    bits endSeqCode = this->bitBuffer->consume(32);

    if (endSeqCode == 0x1b7) { // does equal to end sequence code?
        std::cout << "decode successfully" << std::endl;
    } else {
        std::cout << "decode fail" << std::endl;
    }

    return this->seq;
}

void MpegDecoder::decodeSeqHeader() {

    this->bitBuffer->skip(32); // seq header
    unsigned int displayWidth = this->bitBuffer->consume(12);
    unsigned int displayHeight = this->bitBuffer->consume(12);

    this->bitBuffer->skip(4); // skip aspect ratio

    if (this->seq != nullptr) {
        delete this->seq;
    }

    this->seq = new Sequence(displayWidth, displayHeight, MpegDecoder::FPS_TABLE[(int)this->bitBuffer->consume(4)]);
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

        //realtime display
        if (this->seq->hasDisplayPicture()) {
            this->renderer->render(this->seq->currentDisplayPicture(), this->seq->getFps(), this->seq->getHeight(), this->seq->getWidth());
        }
    } while (this->bitBuffer->nextStartCodeLastByteCompare(StartCodeLastByte::PIC));

}

void MpegDecoder::decodePicture() {
    this->bitBuffer->skip(32); // picture start code
    uint16 tmpRef = (uint16) this->bitBuffer->consume(10);
    byte picType = (byte) this->bitBuffer->consume(3);
    if (picType == Picture::Type::FORBIDDEN || picType == Picture::Type::D) {
        std::cout << "picture type error" << std::endl;
        std::cout << picType << std::endl;
        return;
    }
    this->seq->newPicture(picType, tmpRef);
    this->bitBuffer->skip(16); // vbv delay

    if (picType == Picture::Type::P || picType == Picture::Type::B) {
        this->consumePicForwardBackwardCode(this->seq->picTempInfo.fullPelForwardVec, this->seq->picTempInfo.forwardRSize, this->seq->picTempInfo.forwradF);
    }

    if (picType == Picture::Type::B) {
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
    bool firstMB = true;
    do {
        decodeMacroblock(firstMB);
        firstMB = false;
    } while (!this->bitBuffer->nextBitsCompare(0, 23));
    // next start code is combined with the do-while loop in decodePicture
}

void MpegDecoder::sliceBeginReset() {
    this->seq->mbTempInfo.address = (this->seq->sliceTempInfo.sliceVerticalPosition - 1) * this->seq->getMBWidth() - 1;
    this->seq->mbTempInfo.pastIntraAddress = -2;

    // according to the standard 2.4.4.1
    this->seq->blockTempInfo.resetDcPast();

    // according to the standard 2.4.4.2
    this->seq->mbTempInfo.reconForVec.resetToZero();
    this->seq->mbTempInfo.preReconForVec.resetToZero();

    // according to the standard 2.4.4.3
    this->seq->mbTempInfo.reconBackVec.resetToZero();
    this->seq->mbTempInfo.preReconBackVec.resetToZero();
}

void MpegDecoder::decodeMacroblock(bool firstMB) {
    while (this->bitBuffer->nextBitsCompare(0x0f, 11)) {
        this->bitBuffer->skip(11); // macroblock stuffing
    }

    int mbAddressIncrement = 0;
    while (this->bitBuffer->nextBitsCompare(0x08, 11)) {
        this->bitBuffer->skip(11); // macroblock escape
        mbAddressIncrement += 33;
    }

    mbAddressIncrement += HuffmanTable::decode(HuffmanTable::MACROBLOCK_ADDRESS_INCREMENT, this->bitBuffer);

    // when encountering skipped macroblock. note we have to check it's not the first macroblock on slice
    if (mbAddressIncrement > 1 && !firstMB) {
        this->skippedMacroblockReset();
        this->fillSkippedMacroblocks(mbAddressIncrement);
    } else {
        this->seq->mbTempInfo.address += mbAddressIncrement;
    }

    byte type = (byte) HuffmanTable::decode(HuffmanTable::MACROBLOCK_TYPE_LIST[this->seq->currentPicture().getType()],
                                            this->bitBuffer);
    this->decodeMacroblockType(type);
    if (this->seq->mbTempInfo.quant) {
        this->seq->mbTempInfo.quantScale = (byte) this->bitBuffer->consume(5);
    }

    byte mbIntra = this->seq->mbTempInfo.intra;
    // NOTE, if macroblock is intra, there is no motion information. This can be verified in type VLC table
    //so we skip checking motionForward and motionBackward flags if the macroblock is intra
    if (mbIntra) {
        //no motion inforamtion, according to the standard 2.4.4.2, reset reconstructed motion vectors to zeros
        this->seq->mbTempInfo.reconForVec.resetToZero();
        this->seq->mbTempInfo.preReconForVec.resetToZero();

        // according to the standard 2.4.4.3
        this->seq->mbTempInfo.reconBackVec.resetToZero();
        this->seq->mbTempInfo.preReconBackVec.resetToZero();

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

        } else if (this->seq->currentPictureTypeEquals(Picture::Type::P)) {
            // no motion information in p frame, reset motion vectors to 0
            this->seq->mbTempInfo.reconForVec.resetToZero();
            this->seq->mbTempInfo.preReconForVec.resetToZero();

        }  else if (this->seq->currentPictureTypeEquals(Picture::Type::B)) {
            // according to the standard 2.4.4.3
            // note, it should not be used in building current macroblock, the standard is misleading
            this->seq->mbTempInfo.reconForVec = this->seq->mbTempInfo.preReconForVec;
        }

        if (this->seq->mbTempInfo.motionBackward) {
            // backward H
            this->decodeReconMotionVec(picTempInfo.backwardF, picTempInfo.backwardRSize, picTempInfo.fullPelBackwardVec,
                                       this->seq->mbTempInfo.reconBackVec.hComp,
                                       this->seq->mbTempInfo.preReconBackVec.hComp);
            // backward V
            this->decodeReconMotionVec(picTempInfo.backwardF, picTempInfo.backwardRSize, picTempInfo.fullPelBackwardVec,
                                       this->seq->mbTempInfo.reconBackVec.vComp,
                                       this->seq->mbTempInfo.preReconBackVec.vComp);

        } else if (this->seq->currentPictureTypeEquals(Picture::Type::B)) {
            // according to the standard 2.4.4.3
            // NOTE, it should not be used in building current macroblock, the standard is misleading
            this->seq->mbTempInfo.reconBackVec = this->seq->mbTempInfo.preReconBackVec;
        }

        this->fillMotionMacroblock(this->seq->mbTempInfo.reconForVec, this->seq->mbTempInfo.reconBackVec);
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

    if (mbIntra) {
        //update the last intra macroblock address
        this->seq->mbTempInfo.pastIntraAddress = this->seq->mbTempInfo.address;
    }

//    impossible being D frame
//
//    if (this->seq->currentPictureTypeEquals( Picture::Type::D)) {
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

    if (this->seq->currentPictureTypeEquals(Picture::Type::P)) {
        // set reconstructed motion vectors to zero
        this->seq->mbTempInfo.reconForVec.resetToZero();
        this->seq->mbTempInfo.preReconForVec.resetToZero();
    }

//    if (this->seq->currentPictureTypeEquals(Picture::Type::B)) {
//        // according to the standard 2.4.4.4
//        // Encountering skipped macroblock, it will have the same motion type as the prior macroblock in B frame
//        // The macroblock decoding part will handle this, so we don't have to do any reset for motion vectors
//    }
}

void MpegDecoder::decodeBlock(int blockI, byte mbIntra) {
    this->setZeros(dctZZ, 64);

    int run, level, i = 0;

    if (mbIntra) {
        const int *sizeTable = blockI < 4 ? HuffmanTable::DC_SIZE_LUMINANCE : HuffmanTable::DC_SIZE_CHROMINANCE;
        byte dcSize = (byte) HuffmanTable::decode(sizeTable, this->bitBuffer);
        if (dcSize) {
            byte dcDiff = (byte) bitBuffer->consume(dcSize);
            if (dcDiff & (byte)(1 << (dcSize - 1))) {
                this->dctZZ[0] = dcDiff;
            } else {
                this->dctZZ[0] = (-1 << (dcSize)) | (dcDiff + 1);
            }
        }

    } else {
        uint16 coeffFirst = (uint16) HuffmanTable::decode(HuffmanTable::DCT_COEFF_FIRST, this->bitBuffer);
        this->decodeRunLenCoeff(coeffFirst, run, level);
        i = run;
        this->dctZZ[i] = level;

    }

    // skip checking picture type is not D type
    while (!this->bitBuffer->nextBitsCompare(2, 2)) {
        uint16 coeffNext = (uint16) HuffmanTable::decode(HuffmanTable::DCT_COEFF_NEXT, this->bitBuffer);
        this->decodeRunLenCoeff(coeffNext, run, level);
        i += run + 1;
        this->dctZZ[i] = level;
    }

    this->bitBuffer->skip(2); // end of block = 2

    //start decoding read data
    int *dctRecon = new int[64];

    Block* targetBlock;
    unsigned int mbCol = this->seq->mbTempInfo.mbCol();
    unsigned int mbRow = this->seq->mbTempInfo.mbRow();
    if (mbIntra) {
        int &dcPast = blockI < 4 ? this->seq->blockTempInfo.dcYPast : (blockI == 4 ? this->seq->blockTempInfo.dcCbPast
                                                                                   : this->seq->blockTempInfo.dcCrPast);
        this->reconIntraDctCoef(blockI, dctRecon, this->dctZZ, dcPast);
        this->fidct.doFIDCT(dctRecon);
        targetBlock = this->seq->currentPicture().getBlock(mbCol, mbRow, blockI);
        targetBlock->set(dctRecon);
    } else {
        this->reconNonIntraDctCoef(dctRecon, this->dctZZ);
        this->fidct.doFIDCT(dctRecon);
        targetBlock = this->seq->currentPicture().getBlock(mbCol, mbRow, blockI);
        targetBlock->add(dctRecon);
    }

    delete dctRecon;
    delete targetBlock;
}

void MpegDecoder::reconIntraDctCoef(int blockI, int *dctRecon, int *dctZZ, int &dcPast) {
    for (int i = 0; i < 64; i++) {
        dctRecon[i] = (2 * dctZZ[ZIG_ZAG_ORDER[i]] * this->seq->mbTempInfo.quantScale * this->intraQuantMat[i]) / 16;
        if ((dctRecon[i] & 1) == 0) {
            dctRecon[i] -= this->sign(dctRecon[i]);
        }
        if (dctRecon[i] > 2047) {
            dctRecon[i] = 2047;
        } else if (dctRecon[i] < -2048) {
            dctRecon[i] = -2048;
        }
    }
    dctRecon[0] = this->dctZZ[0] * 8;
    if (blockI < 1 || blockI > 3) { //block 0, 4, 5
        if (this->seq->mbTempInfo.address - this->seq->mbTempInfo.pastIntraAddress > 1) {
            dctRecon[0] += 128 * 8;
        } else {
            dctRecon[0] += dcPast;
        }
    } else {
        dctRecon[0] += dcPast;
    }
    dcPast = dctRecon[0];
}

void MpegDecoder::reconNonIntraDctCoef(int *dctRecon, int *dctZZ) {
    for (int i = 0; i < 64; i++) {
        int zi = ZIG_ZAG_ORDER[i];
        dctRecon[i] = ((2 * dctZZ[zi] + this->sign(dctZZ[zi])) * this->seq->mbTempInfo.quantScale * this->nonIntraQuantMat[i]) / 16;
        if ((dctRecon[i] & 1) == 0) {
            dctRecon[i] -= this->sign(dctRecon[i]);
        }
        if (dctRecon[i] > 2047) {
            dctRecon[i] = 2047;
        } else if (dctRecon[i] < -2048) {
            dctRecon[i] = -2048;
        } // else if (dctZZ[zi] == 0) {
//            dctRecon[i] = 0;
//        }
    }
}

void MpegDecoder::decodeMacroblockType(byte type) {
    this->seq->mbTempInfo.quant = (byte) ((type & 0x10) >> 4);
    this->seq->mbTempInfo.motionForward = (byte) ((type & 0x08) >> 3);
    this->seq->mbTempInfo.motionBackward = (byte) ((type & 0x04) >> 2) ;
    this->seq->mbTempInfo.pattern = (byte) ((type & 0x02) >> 1);
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

void MpegDecoder::setZeros(int *array, int length) {
    memset(array, 0, length * sizeof(int));
}

void MpegDecoder::decodeRunLenCoeff(uint16 coeff, int& run, int& level) {
    // escape
    if (coeff == 0xffff) {
        run = this->bitBuffer->consume(6);
        level = this->bitBuffer->consume(8);
        if (level == 0) { // [128, 255]
            level = this->bitBuffer->consume(8);
        }
        else if (level == 128) { // [-255, -128]
            level = this->bitBuffer->consume(8) - 256;
        }
        else if (level > 128) { // [-127, 0(forbidden)]
            level = level - 256;
        } // else level = level [1, 127]
    }
    else {
        run = coeff >> 8;
        level = coeff & 0xff;
        if (this->bitBuffer->consumeOneBit()) { //last bit 's' for sign
            level = -level;
        }
    }
}

int MpegDecoder::sign(int val) {
    return (val > 0) - (val < 0);
}


void MpegDecoder::fillSkippedMacroblocks(int increment) {
    Picture& prevPicture = this->seq->pastPicture();
    Picture& curPicture = this->seq->currentPicture();

    for (int i = 0; i < increment - 1; i++) {
        this->seq->mbTempInfo.address++;
        unsigned int mbRow = this->seq->mbTempInfo.mbRow();
        unsigned int mbCol = this->seq->mbTempInfo.mbCol();

        Block* srcY = prevPicture.getMacroblock(mbCol, mbRow, 0);
        Block* destY = curPicture.getMacroblock(mbCol, mbRow, 0);
        Block* srcCb = prevPicture.getMacroblock(mbCol, mbRow, 1);
        Block* destCb = curPicture.getMacroblock(mbCol, mbRow, 1);
        Block* srcCr = prevPicture.getMacroblock(mbCol, mbRow, 2);
        Block* destCr = curPicture.getMacroblock(mbCol, mbRow, 2);

        destY->set(*srcY);
        destCb->set(*srcCb);
        destCr->set(*srcCr);

        delete srcY;
        delete destY;
        delete srcCb;
        delete destCb;
        delete srcCr;
        delete destCr;
    }
    this->seq->mbTempInfo.address++;
}

void MpegDecoder::fillMotionMacroblock(Sequence::MotionVector &reconForVec, Sequence::MotionVector &reconBackVec) {
    unsigned int mbRow = this->seq->mbTempInfo.mbRow();
    unsigned int mbCol = this->seq->mbTempInfo.mbCol();

    Picture& curPicture = this->seq->currentPicture();
    Block* destY = curPicture.getMacroblock(mbCol, mbRow, 0);
    Block* destCb = curPicture.getMacroblock(mbCol, mbRow, 1);
    Block* destCr = curPicture.getMacroblock(mbCol, mbRow, 2);

    bool hasForMotion = this->seq->mbTempInfo.motionForward != 0;
    bool hasBackMotion = this->seq->mbTempInfo.motionBackward != 0;

    if (!hasForMotion && !hasBackMotion) {
        Picture& prevPicture = this->seq->pastPicture();

        Block* srcY = prevPicture.getMacroblock(mbCol, mbRow, 0);
        Block* srcCb = prevPicture.getMacroblock(mbCol, mbRow, 1);
        Block* srcCr = prevPicture.getMacroblock(mbCol, mbRow, 2);

        destY->set(*srcY);
        destCb->set(*srcCb);
        destCr->set(*srcCr);


        delete srcY;
        delete srcCb;
        delete srcCr;

    } else if (hasForMotion && !hasBackMotion) {
        this->fillSingleMotion(this->seq->pastPicture(), mbCol, mbRow, reconForVec, destY, destCb, destCr);

    } else if (!hasForMotion && hasBackMotion) {
        this->fillSingleMotion(this->seq->futurePicture(), mbCol, mbRow, reconBackVec, destY, destCb, destCr);
    } else {
        this->fillSingleMotion(this->seq->pastPicture(), mbCol, mbRow, reconForVec, destY, destCb, destCr);
        this->addAndHalfSingleMotion(this->seq->futurePicture(), mbCol, mbRow, reconBackVec, destY, destCb, destCr);
    }

    delete destY;
    delete destCb;
    delete destCr;
}

void MpegDecoder::fillSingleMotion(Picture &refPicture, unsigned int mbCol, unsigned int mbRow,
                                   Sequence::MotionVector &reconVec, Block *destY,
                                   Block *destCb, Block *destCr) {
    int right = reconVec.hComp >> 1;
    int rightHalf = reconVec.hComp - 2 * right;
    int down = reconVec.vComp >> 1;
    int downHalf = reconVec.vComp - 2 * down;

    //We don't follow the standard when calculating chrominance motion vector,
    // since we upscale the chrominance macroblock beforhand
//    int rightC = (reconVec.hComp / 2) >> 2;
//    int rightHalfC = (reconVec.hComp / 2) - 2 * rightC;
//    int downC = (reconVec.vComp / 2) >> 2;
//    int downHalfC = (reconVec.vComp / 2) - 2 * downC;

    Block** srcBlocks = new Block*[4];

    //fill Y
    if (rightHalf == 1) {
        if (downHalf == 1) {
            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 0, right + 1, down);
            srcBlocks[2] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down + 1);
            srcBlocks[3] = refPicture.getMacroblock(mbCol, mbRow, 0, right + 1, down + 1);

            destY->averageBlocksSet(srcBlocks, 4);
            delete srcBlocks[0];
            delete srcBlocks[1];
            delete srcBlocks[2];
            delete srcBlocks[3];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 1, right + 1, down);
            srcBlocks[2] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down + 1);
            srcBlocks[3] = refPicture.getMacroblock(mbCol, mbRow, 1, right + 1, down + 1);

            destCb->averageBlocksSet(srcBlocks, 4);
            delete srcBlocks[0];
            delete srcBlocks[1];
            delete srcBlocks[2];
            delete srcBlocks[3];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 2, right + 1, down);
            srcBlocks[2] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down + 1);
            srcBlocks[3] = refPicture.getMacroblock(mbCol, mbRow, 2, right + 1, down + 1);

            destCr->averageBlocksSet(srcBlocks, 4);
            delete srcBlocks[0];
            delete srcBlocks[1];
            delete srcBlocks[2];
            delete srcBlocks[3];
        } else {
            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 0, right + 1, down);

            destY->averageBlocksSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 1, right + 1, down);

            destCb->averageBlocksSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 2, right + 1, down);

            destCr->averageBlocksSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];
        }
    } else {
        if (downHalf == 1) {
            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down + 1);

            destY->averageBlocksSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down + 1);

            destCb->averageBlocksSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down + 1);

            destCr->averageBlocksSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];
        } else {
            Block* srcBlock = refPicture.getMacroblock(mbCol, mbRow, 0, right, down);
            destY->set(*srcBlock);
            delete srcBlock;

            srcBlock = refPicture.getMacroblock(mbCol, mbRow, 1, right, down);
            destCb->set(*srcBlock);
            delete srcBlock;

            srcBlock = refPicture.getMacroblock(mbCol, mbRow, 2, right, down);
            destCr->set(*srcBlock);
            delete srcBlock;
        }
    }

    delete srcBlocks;

}

// add the pel value and then divided by 2
void MpegDecoder::addAndHalfSingleMotion(Picture &refPicture, unsigned int mbCol, unsigned int mbRow,
                                   Sequence::MotionVector &reconVec, Block *destY,
                                   Block *destCb, Block *destCr) {
    int right = reconVec.hComp >> 1;
    int rightHalf = reconVec.hComp - 2 * right;
    int down = reconVec.vComp >> 1;
    int downHalf = reconVec.vComp - 2 * down;

    //We don't follow the standard when calculating chrominance motion vector,
    // since we upscale the chrominance macroblock beforhand
//    int rightC = (reconVec.hComp / 2) >> 2;
//    int rightHalfC = (reconVec.hComp / 2) - 2 * rightC;
//    int downC = (reconVec.vComp / 2) >> 2;
//    int downHalfC = (reconVec.vComp / 2) - 2 * downC;

    Block** srcBlocks = new Block*[4];

    //fill Y
    if (rightHalf == 1) {
        if (downHalf == 1) {
            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 0, right + 1, down);
            srcBlocks[2] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down + 1);
            srcBlocks[3] = refPicture.getMacroblock(mbCol, mbRow, 0, right + 1, down + 1);

            destY->addAverageBlocksAndHalfSet(srcBlocks, 4);
            delete srcBlocks[0];
            delete srcBlocks[1];
            delete srcBlocks[2];
            delete srcBlocks[3];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 1, right + 1, down);
            srcBlocks[2] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down + 1);
            srcBlocks[3] = refPicture.getMacroblock(mbCol, mbRow, 1, right + 1, down + 1);

            destCb->addAverageBlocksAndHalfSet(srcBlocks, 4);
            delete srcBlocks[0];
            delete srcBlocks[1];
            delete srcBlocks[2];
            delete srcBlocks[3];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 2, right + 1, down);
            srcBlocks[2] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down + 1);
            srcBlocks[3] = refPicture.getMacroblock(mbCol, mbRow, 2, right + 1, down + 1);

            destCr->addAverageBlocksAndHalfSet(srcBlocks, 4);
            delete srcBlocks[0];
            delete srcBlocks[1];
            delete srcBlocks[2];
            delete srcBlocks[3];
        } else {
            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 0, right + 1, down);

            destY->addAverageBlocksAndHalfSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 1, right + 1, down);

            destCb->addAverageBlocksAndHalfSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 2, right + 1, down);

            destCr->addAverageBlocksAndHalfSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];
        }
    } else {
        if (downHalf == 1) {
            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 0, right, down + 1);

            destY->addAverageBlocksAndHalfSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 1, right, down + 1);

            destCb->addAverageBlocksAndHalfSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];

            srcBlocks[0] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down);
            srcBlocks[1] = refPicture.getMacroblock(mbCol, mbRow, 2, right, down + 1);

            destCr->addAverageBlocksAndHalfSet(srcBlocks, 2);
            delete srcBlocks[0];
            delete srcBlocks[1];
        } else {
            Block* srcBlock = refPicture.getMacroblock(mbCol, mbRow, 0, right, down);
            destY->addAndHalfSet(*srcBlock);
            delete srcBlock;

            srcBlock = refPicture.getMacroblock(mbCol, mbRow, 1, right, down);
            destCb->addAndHalfSet(*srcBlock);
            delete srcBlock;

            srcBlock = refPicture.getMacroblock(mbCol, mbRow, 2, right, down);
            destCr->addAndHalfSet(*srcBlock);
            delete srcBlock;
        }
    }

    delete srcBlocks;

}

















