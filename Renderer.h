//
// Created by maeglin89273 on 6/20/17.
//

#ifndef ITCT_MPEG1_RENDERER_H
#define ITCT_MPEG1_RENDERER_H


#include "Picture.h"

// A interface for picture rendering
class Renderer {
public:
    virtual void render(Picture &yccPicture, float fps, unsigned int height, unsigned int width) = 0;
    virtual ~Renderer(){};
};


#endif //ITCT_MPEG1_RENDERER_H
