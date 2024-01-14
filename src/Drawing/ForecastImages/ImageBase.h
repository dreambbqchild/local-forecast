#ifndef IMAGEBASE_H
#define IMAGEBASE_H

#include <stdint.h>

const uint32_t defaultImageWidth = 1060;
const uint32_t defaultImageHeight = 1100;

class ImageBase {
protected:
    static const uint32_t totalWidth = defaultImageWidth, totalHeight = defaultImageHeight;
};

#endif