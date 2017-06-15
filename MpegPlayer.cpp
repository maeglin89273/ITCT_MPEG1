//
// Created by maeglin89273 on 2017/6/15.
//

#include "MpegPlayer.h"

MpegPlayer::MpegPlayer(char* fileName):
        bitBuffer(BitBuffer(fileName)),
        decoder(MpegDecoder(&this->bitBuffer)) {

}

void MpegPlayer::play() {
    this->decoder.decode();

}
