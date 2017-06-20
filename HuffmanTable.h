//
// Created by maeglin89273 on 6/16/17.
//

#ifndef ITCT_MPEG1_HUFFMANTREE_H
#define ITCT_MPEG1_HUFFMANTREE_H

#include "TypeDefinition.h"
#include "BitBuffer.h"

// Since writing own tables is tedious and error-prone, I refer the huffman tables from javascript mpeg1:
// https://github.com/phoboslab/jsmpeg/blob/master/src/mpeg1.js

class HuffmanTable {

public:
    const static int MACROBLOCK_ADDRESS_INCREMENT[];
    const static int MACROBLOCK_TYPE_I[];
    const static int MACROBLOCK_TYPE_P[];
    const static int MACROBLOCK_TYPE_B[];
    const static int CODE_BLOCK_PATTERN[];
    const static int MOTION[];
    const static int DC_SIZE_LUMINANCE[];
    const static int DC_SIZE_CHROMINANCE[];
    const static int DCT_COEFF_FIRST[];
    const static int DCT_COEFF_NEXT[];
    const static int* MACROBLOCK_TYPE_LIST[];

    static int decode(const int* table, BitBuffer *buffer);

private:
    HuffmanTable() {
        //do not initialize this class
    }
};


#endif //ITCT_MPEG1_HUFFMANTREE_H
