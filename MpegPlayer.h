//
// Created by maeglin89273 on 2017/6/15.
//

#ifndef ITCT_MPEG1_MPEGPLAYER_H
#define ITCT_MPEG1_MPEGPLAYER_H


#include "BitBuffer.h"
#include "MpegDecoder.h"
#include "Renderer.h"
#include <opencv2/opencv.hpp>

// The class for managing the buffer and the decoder.
// It's also responsible for displaying the picture.
class MpegPlayer: public Renderer {

public:
    MpegPlayer(char* fileName);
    ~MpegPlayer();
    Sequence * play();
    void writeSequence(Sequence *seq);
    void render(Picture &yccPicture, float fps, unsigned int height, unsigned int width) override;
    bool isFileLoadSucess();
private:
    BitBuffer* bitBuffer;
    MpegDecoder* decoder;

    //the display cache
    byte* picData;
    long timestamp;
};


#endif //ITCT_MPEG1_MPEGPLAYER_H
