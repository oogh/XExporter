//
//  XImage.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XIMAGE_H
#define XEXPORTER_XIMAGE_H

#include <memory>

struct XImage {
    uint8_t* pixels = nullptr;

    int width = 0;

    int height = 0;

    XImage(): pixels(nullptr), width(0), height(0) {
    }

    void allocBuffer(int w, int h) {
        if (this->width != w || this->height != h || !pixels) {
            freeBuffer();
            this->width = w;
            this->height = h;
            pixels = new uint8_t[w * h * 4];
        }
    }

    void copyPixels(uint8_t* src, int w, int h) {
        allocBuffer(w, h);
        memcpy(this->pixels, src, w * h * 4);
    }

    void freeBuffer() {
        if (this->pixels) {
            delete[] this->pixels;
            this->pixels = nullptr;
        }
    }

    ~XImage() {
        freeBuffer();
        this->width = 0;
        this->height = 0;
    }
};

#endif //XEXPORTER_XIMAGE_H
