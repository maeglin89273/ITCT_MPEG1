#include <iostream>
#include "MpegPlayer.h"

int main(int argc, char** argv) {
    if ( argc != 2 )
    {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }

    MpegPlayer player(argv[1]);

    return 0;
}