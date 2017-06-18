//
// Created by maeglin89273 on 2017/6/15.
//

#include "MpegPlayer.h"

MpegPlayer::MpegPlayer(char* fileName):
        bitBuffer(BitBuffer(fileName)),
        decoder(MpegDecoder(&this->bitBuffer)) {

}

void MpegPlayer::play() {
    Sequence* seq = this->decoder.decode();
    this->writeSeq(seq);
}

void MpegPlayer::writeSeq(Sequence *seq) {
    Picture** pics = seq->getPictureArray();
    Picture* pic;
    std:: string fileStart = "./seq/pic_";
    std:: string fileEnd = ".jpg";
    unsigned int len = seq->length();
    for (unsigned int i = 0; i < len; i++) {
        pic = pics[i];
        cv::Mat imageMat(pic->getHeight(), pic->getWidth(), CV_8UC3, pic->getData());
        cv::imwrite( fileStart + std::to_string(i) + fileEnd, imageMat);
    }

}
