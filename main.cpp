#include <iostream>
#include "MpegPlayer.h"
#include <opencv2/opencv.hpp>

int main(int argc, char** argv) {
    if ( argc != 2 )
    {
        printf("usage: Mpeg1Decoder <Video_Path>\n");
        return -1;
    }

    MpegPlayer player(argv[1]);
    if (player.isFileLoadSucess()) {
        Sequence *seq = player.play();
    }
//    player.writeSequence(seq);

    return 0;
}