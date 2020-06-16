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

enum ImageType {
    IMG_TYPE_UNKNOWN = -1,
    IMG_TYPE_YUV420P = 0, // AV_PIX_FMT_YUV420P
    IMG_TYPE_RGB24 = 2, // AV_PIX_FMT_RGB24
    IMG_TYPE_RGBA = 26, // AV_PIX_FMT_RGBA
};

struct XImage {
    uint8_t* pixels = nullptr;

    int width = 0;

    int height = 0;

    long pts = -1;

    long duration = -1;

    int format = -1;

    XImage(): pixels(nullptr), width(0), height(0), pts(-1), duration(-1) {

    }

    ~XImage() {
        freeBuffer();
        this->width = 0;
        this->height = 0;
        this->pts = -1;
        this->duration = -1;
    }

    void allocBuffer(int w, int h, int fmt) {
        if (this->width != w || this->height != h || !pixels) {
            freeBuffer();
            this->width = w;
            this->height = h;
            if (fmt == IMG_TYPE_YUV420P) {
                pixels = pixels = new uint8_t[w * h * 3 / 2];
            } else if (fmt == IMG_TYPE_RGB24) {
                pixels = pixels = new uint8_t[w * h * 3];
            } else if (fmt == IMG_TYPE_RGBA) {
                pixels = new uint8_t[w * h * 4];
            }
        }
    }

    void copyPixels(uint8_t* src, int w, int h) {
        allocBuffer(w, h, IMG_TYPE_RGBA);
        memcpy(this->pixels, src, w * h * 4);
    }

    void freeBuffer() {
        if (this->pixels) {
            delete[] this->pixels;
            this->pixels = nullptr;
        }
    }

private:
    int getBufferSize(int w, int h, int fmt) {
        int result = 0;
        if (fmt == IMG_TYPE_YUV420P) {
            result = w * h * 3 / 2;
        } else if (fmt == IMG_TYPE_RGB24) {
            result = w * h * 3;
        } else if (fmt == IMG_TYPE_RGBA) {
            result = w * h * 4;
        }
        return result;
    }
};

#endif //XEXPORTER_XIMAGE_H
