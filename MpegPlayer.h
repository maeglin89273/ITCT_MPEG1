//
// Created by maeglin89273 on 2017/6/15.
//

#ifndef ITCT_MPEG1_MPEGPLAYER_H
#define ITCT_MPEG1_MPEGPLAYER_H


#include "BitBuffer.h"
#include "MpegDecoder.h"
#include <opencv2/opencv.hpp>

class MpegPlayer {

public:
    MpegPlayer(char* fileName);
    void play();
private:
    BitBuffer bitBuffer;
    MpegDecoder decoder;
//    Display display;
    void writeSeq(Sequence *seq);
};


#endif //ITCT_MPEG1_MPEGPLAYER_H
