//
//  XExporter.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XEXPORTER_H
#define XEXPORTER_XEXPORTER_H

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include "XFFHeader.h"

class XFrameQueue;

enum ExportResult {
    EXPORT_RESULT_FAILED = -1,
    EXPORT_RESULT_CANCELED,
    EXPORT_RESULT_SUCCEED,
};

class XExporter {
    using ExportResultCallback = std::function<void(ExportResult result)>;

public:
    XExporter(const std::string& outputPath,
              int width = DEFAULT_PARAM_WIDTH,
              int height = DEFAULT_PARAM_HEIGHT,
              int fps = DEFAULT_PARAM_FPS,
              long duration = DEFAULT_PARAM_DURATION);

    ~XExporter();

    void setVideoDisable(bool disableVideo);

    void setAudioDisable(bool disableAudio);

    void setExportResultCallback(ExportResultCallback resultCallback = nullptr);

    void start();

    int encodeFrame(uint8_t* pixels, int width, int height);

    int encodeSample(uint8_t* samples);

    void stop();

    void debug();

private:

    int openOutFile();

    int addVideoStream();

    int addAudioStream();

    int writeVideoFrame();

    int writeAudioFrame();

    int closeOutFile();

private:

    std::shared_ptr<Frame> allocVideoFrame();

    std::shared_ptr<Frame> allocAudioFrame();

    void frameConvert(std::shared_ptr<Frame> dst, uint8_t* src, int srcWidth, int srcHeight);

    void sampleCovert(std::shared_ptr<Frame> dst, uint8_t* src);

private:
    void encodeVideoWorkThread(void* opaque);

    void encodeAudioWorkThread(void* opaque);

private:
    static const int DEFAULT_PARAM_WIDTH = 540;
    static const int DEFAULT_PARAM_HEIGHT = 960;
    static const int DEFAULT_PARAM_FPS = 25;
    static const long DEFAULT_PARAM_DURATION = 10000;

    const AVPixelFormat EXPORT_PARAM_PIX_FMT = AV_PIX_FMT_YUV420P;
    const int EXPORT_PARAM_SAMPLE_RATE = 44100;
    const AVSampleFormat EXPORT_PARAM_SAMPLE_FMT = AV_SAMPLE_FMT_S16;
    const uint64_t EXPORT_PARAM_CHANNEL_LAYOUT = AV_CH_LAYOUT_STEREO;

private:
    std::string mOutputPath;
    int mWidth;
    int mHeight;
    int mFPS;
    long mDuration;

    bool mDisableAudio;
    bool mDisableVideo;

    std::unique_ptr<AVFormatContext, OutputFormatDeleter> mFormatCtx;
    int mAudioIndex;
    std::unique_ptr<AVCodecContext, CodecDeleter> mAudioCodecCtx;
    std::unique_ptr<SwrContext, SwrContextDeleter> mSwrContext;
    int mVideoIndex;
    std::unique_ptr<AVCodecContext, CodecDeleter> mVideoCodecCtx;
    std::unique_ptr<SwsContext, SwsContextDeleter> mSwsContext;

    std::unique_ptr<XFrameQueue> mFrameQueue;

    ExportResultCallback mResultCallback;

    std::unique_ptr<std::thread> mEncodeAudioTid;
    std::unique_ptr<std::thread> mEncodeVideoTid;

    std::mutex mMutex;

    bool mAborted;

    int mSendFrameCount;
    int mReceivePacketCount;
};


#endif //XEXPORTER_XEXPORTER_H
