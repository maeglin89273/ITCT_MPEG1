//
// Created by maeglin89273 on 2017/6/15.
//

#ifndef ITCT_MPEG1_MPEGPLAYER_H
#define ITCT_MPEG1_MPEGPLAYER_H


#include "BitBuffer.h"
#include "MpegDecoder.h"

class MpegPlayer {

public:
    MpegPlayer(char* fileName);
    void play();
private:
    BitBuffer bitBuffer;
    MpegDecoder decoder;
//    Display display;
};


#endif //ITCT_MPEG1_MPEGPLAYER_H
