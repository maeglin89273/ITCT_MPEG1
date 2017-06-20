//
// Created by maeglin89273 on 2017/6/15.
//

#include <chrono>
#include "MpegPlayer.h"

using namespace cv;
using namespace std::chrono;
MpegPlayer::MpegPlayer(char* fileName) {
    this->bitBuffer = new BitBuffer(fileName);
    this->decoder = new MpegDecoder(this->bitBuffer, this);
    this->picData = nullptr;
}

Sequence * MpegPlayer::play() {
    namedWindow("Player", WINDOW_AUTOSIZE );
    //record the start time. see the render method
    this->timestamp = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
    return this->decoder->decode();

}

// for saving picture files
void MpegPlayer::writeSequence(Sequence *seq) {

    byte** pics = seq->displaySequenceData();
    byte* pic = nullptr;
    std:: string fileStart = "./seq/pic_";
    std:: string fileEnd = ".jpg";
    unsigned int len = seq->length();
    for (unsigned int i = 0; i < len; i++) {
        pic = pics[i];
        Mat imageMat(seq->getHeight(), seq->getHeight(), CV_8UC3, pic);
        imwrite( fileStart + std::to_string(i) + fileEnd, imageMat);
        delete [] pic;
    }
}

MpegPlayer::~MpegPlayer() {
    delete this->bitBuffer;
    delete this->decoder;
    if(this->picData != nullptr) {
        delete this->picData;
    }
}

void MpegPlayer::render(Picture &yccPicture, float fps, unsigned int height, unsigned int width) {
    if (this->picData == nullptr) {
        this->picData = new byte[3 * height * width];
    }

    int spf = (int)round(1000 / fps); // milliseconds per frame
    //convert to opencv format
    yccPicture.toBGRAndCrop(this->picData, height, width);
    Mat imageMat(height, width, CV_8UC3, this->picData);

    long curTimestamp = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();

    //calculate how many milliseconds are needed to compensate the required FPS
    int pauseTime = spf - (int)(curTimestamp - this->timestamp);
    if (pauseTime <= 0) {
        pauseTime = 1; // pause time should not be less than one
    }

    waitKey(pauseTime);
    imshow("Player", imageMat);

    // update the timestamp for next round
    this->timestamp = curTimestamp;

}

bool MpegPlayer::isFileLoadSucess() {
    return this->bitBuffer->isLaodSuccess();
}
