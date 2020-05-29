//
//  XExporter.cpp
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XExporter.h"
#include "XThreadUtils.h"
#include "XFrameQueue.h"

void dumpPacket(const AVFormatContext *ic, const AVPacket* pkt) {
    AVRational *time_base = &ic->streams[pkt->stream_index]->time_base;

    av_log(nullptr, AV_LOG_INFO, "[XExporter] pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

XExporter::XExporter(const std::string &outputPath, int width, int height, int fps, long duration)
        : mOutputPath(outputPath), mWidth(width), mHeight(height), mFPS(fps), mDuration(duration),
          mDisableAudio(false), mDisableVideo(false), mAborted(false), mSendFrameCount(0), mReceivePacketCount(0) {

}

XExporter::~XExporter() {
    if (!mAborted) {
        stop();
    }
}

void XExporter::start() {
    int ret = openOutFile();
    if (ret < 0) {
        if (mResultCallback) {
            mResultCallback(EXPORT_RESULT_FAILED);
        }
        return;
    }

    if (!mDisableAudio) {
        mEncodeAudioTid = std::make_unique<std::thread>(std::bind(&XExporter::encodeAudioWorkThread, this, this));
    }

    if (!mDisableVideo) {
        mEncodeVideoTid = std::make_unique<std::thread>(std::bind(&XExporter::encodeVideoWorkThread, this, this));
    }
}

int XExporter::encodeSample(uint8_t* samples) {
    if (!samples) {
        return -1;
    }

    return 0;
}

int XExporter::encodeFrame(uint8_t *pixels, int width, int height) {
    if (!pixels || width < 0 || height < 0) {
        return -1;
    }

    auto frame = allocVideoFrame();
    if (!frame) {
        return -1;
    }

    frameConvert(frame, pixels, width, height);

    mFrameQueue->put(frame);

    return 0;
}

void XExporter::stop() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mAborted = true;
        mFrameQueue->signal();
    }

    int ret = closeOutFile();
    if (ret < 0) {
        if (mResultCallback) {
            mResultCallback(EXPORT_RESULT_FAILED);
        }
        return;
    }

    if (mResultCallback) {
        mResultCallback(EXPORT_RESULT_SUCCEED);
    }
}

void XExporter::encodeAudioWorkThread(void* opaque) {
    XThreadUtils::configThreadName("encodeAudioWorkThread");
    auto exporter = reinterpret_cast<XExporter *>(opaque);
    av_log(nullptr, AV_LOG_INFO, "[XExporter] encodeAudioWorkThread ++++\n");
    for (;;) {
        if (exporter->mAborted) {
            break;
        }

    }
    av_log(nullptr, AV_LOG_INFO, "[XExporter] encodeAudioWorkThread ----\n");
}

void XExporter::encodeVideoWorkThread(void *opaque) {
    XThreadUtils::configThreadName("encodeVideoWorkThread");
    auto exporter = reinterpret_cast<XExporter *>(opaque);
    av_log(nullptr, AV_LOG_INFO, "[XExporter] encodeVideoWorkThread ++++\n");
    int ret;
    bool needFlush = false;
    for (;;) {
        if (exporter->mAborted) {
            break;
        }

        ret = exporter->encodeVideoFrame();
        if (ret < 0) {
            needFlush = true;
        }

        if (needFlush) {
            ret = avcodec_send_frame(exporter->mVideoCodecCtx.get(), nullptr);
            mSendFrameCount++;
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                break;
            }

            for(;;) {
                auto pkt = std::make_unique<Packet>();
                ret = avcodec_receive_packet(exporter->mVideoCodecCtx.get(), pkt->avpkt);
                if (ret >= 0) {
                    mReceivePacketCount++;
                    av_packet_rescale_ts(pkt->avpkt, exporter->mVideoCodecCtx->time_base,
                                         exporter->mFormatCtx->streams[exporter->mVideoIndex]->time_base);
                    pkt->avpkt->stream_index = exporter->mVideoIndex;
                    dumpPacket(exporter->mFormatCtx.get(), pkt->avpkt);
                    ret = av_interleaved_write_frame(exporter->mFormatCtx.get(), pkt->avpkt);
                    if (ret < 0) {
                        av_log(nullptr, AV_LOG_FATAL, "[XExporter] av_interleaved_write_frame failed: %s\n", av_err2str(ret));
                        break;
                    }
                }
                if (ret == AVERROR_EOF) {
                    break;
                }
            }
        }

    }
    av_log(nullptr, AV_LOG_INFO, "[XExporter] encodeVideoWorkThread ----\n");
}

void XExporter::setExportResultCallback(ExportResultCallback resultCallback) {
    mResultCallback = resultCallback;
}

void XExporter::setAudioDisable(bool disableAudio) {
    mDisableAudio = disableAudio;
}

void XExporter::setVideoDisable(bool disableVideo) {
    mDisableVideo = disableVideo;
}

std::shared_ptr<Frame> XExporter::allocVideoFrame() {
    auto frame = std::make_shared<Frame>();
    if (!frame) {
        av_log(nullptr, AV_LOG_FATAL, "[XEncoder] allocVideoFrame failed: Out of memory\n");
        return nullptr;
    }
    frame->avframe->width = mWidth;
    frame->avframe->height = mHeight;
    frame->avframe->format = EXPORT_PARAM_PIX_FMT;

    int ret = av_frame_get_buffer(frame->avframe, 0);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XEncoder] allocVideoFrame failed: %s\n", av_err2str(ret));
        return nullptr;
    }

    return frame;
}

void XExporter::frameConvert(std::shared_ptr<Frame> dst, uint8_t *src, int srcWidth, int srcHeight) {
    if (!mSwsContext) {
        SwsContext *sws = sws_getCachedContext(nullptr, srcWidth, srcHeight, AV_PIX_FMT_RGBA,
                                               mWidth, mHeight, EXPORT_PARAM_PIX_FMT,
                                               0, nullptr, nullptr, nullptr);
        if (!sws) {
            av_log(nullptr, AV_LOG_ERROR, "[XExporter] sws_getCachedContext failed!\n");
            return;
        }
        mSwsContext = std::unique_ptr<SwsContext, SwsContextDeleter>(sws);
    }

    uint8_t *data[4] = {src, nullptr};
    int linesize[4] = {0};
    av_image_fill_linesizes(linesize, AV_PIX_FMT_RGBA, srcWidth);
    sws_scale(mSwsContext.get(), data, linesize, 0, srcHeight, dst->avframe->data, dst->avframe->linesize);
}

int XExporter::openOutFile() {
    AVFormatContext *ic = nullptr;
    int ret = avformat_alloc_output_context2(&ic, nullptr, nullptr, mOutputPath.data());
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] avformat_alloc_output_context2 failed: %s\n", av_err2str(ret));
        return ret;
    }

    mFormatCtx = std::unique_ptr<AVFormatContext, OutputFormatDeleter>(ic);

    if (!mDisableAudio) {
        ret = addAudioStream();
        if (ret < 0) {
            return ret;
        }
    }

    if (!mDisableVideo) {
        ret = addVideoStream();
        if (ret < 0) {
            return ret;
        }
        mFrameQueue = std::make_unique<XFrameQueue>();
    }

    ret = avio_open(&ic->pb, mOutputPath.data(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] avio_open failed: %s\n", av_err2str(ret));
        return ret;
    }

    ret = avformat_write_header(ic, nullptr);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] avformat_write_header failed: %s\n", av_err2str(ret));
        return ret;
    }

    return 0;
}

int XExporter::addAudioStream() {

    if (!mFormatCtx) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] Output File has not open!\n");
        return -1;
    }

    AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
    if (!codec) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] cannot find encoder: libfdk-aac\n");
        return AVERROR_ENCODER_NOT_FOUND;
    }

    AVStream *stream = avformat_new_stream(mFormatCtx.get(), codec);
    if (!stream) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] avformat new audio stream failed!\n");
        return AVERROR(ENOMEM);
    }
    mAudioIndex = stream->index;
    stream->time_base = {1, EXPORT_PARAM_SAMPLE_RATE};

    AVCodecContext *avctx = avcodec_alloc_context3(codec);
    if (!avctx) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] audio codec alloc context failed!\n");
        return AVERROR(ENOMEM);
    }

    mAudioCodecCtx = std::unique_ptr<AVCodecContext, CodecDeleter>(avctx);

    avctx->frame_size = 1024;
    avctx->sample_fmt = EXPORT_PARAM_SAMPLE_FMT;
    avctx->sample_rate = EXPORT_PARAM_SAMPLE_RATE;
    avctx->channel_layout = EXPORT_PARAM_CHANNEL_LAYOUT;
    avctx->channels = av_get_channel_layout_nb_channels(avctx->channel_layout);
    avctx->time_base = {1, avctx->sample_rate};

    if (mFormatCtx->oformat->flags | AVFMT_GLOBALHEADER) {
        avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    int ret = avcodec_parameters_from_context(stream->codecpar, avctx);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] avcodec_parameters_from_context failed: %s\n", av_err2str(ret));
        return ret;
    }

    ret = avcodec_open2(avctx, nullptr, nullptr);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] avcodec_open2 failed: %s\n", av_err2str(ret));
        return ret;
    }

    return 0;
}

int XExporter::addVideoStream() {
    if (!mFormatCtx) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] Output File has not open!\n");
        return -1;
    }

    AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] cannot find encoder: libx264\n");
        return AVERROR_ENCODER_NOT_FOUND;
    }

    AVStream *stream = avformat_new_stream(mFormatCtx.get(), codec);
    if (!stream) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] avformat new video stream failed!\n");
        return AVERROR(ENOMEM);
    }
    mVideoIndex = stream->index;
    stream->time_base = {1, mFPS};

    AVCodecContext *avctx = avcodec_alloc_context3(codec);
    if (!avctx) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] video codec alloc context failed!\n");
        return AVERROR(ENOMEM);
    }
    mVideoCodecCtx = std::unique_ptr<AVCodecContext, CodecDeleter>(avctx);

    avctx->width = mWidth;
    avctx->height = mHeight;
    avctx->pix_fmt = EXPORT_PARAM_PIX_FMT;
    avctx->time_base = {1, mFPS};

    if (mFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    int ret = avcodec_open2(avctx, nullptr, nullptr);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] avcodec_open2 failed: %s\n", av_err2str(ret));
        return ret;
    }

    ret = avcodec_parameters_from_context(stream->codecpar, avctx);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] avcodec_parameters_from_context failed: %s\n", av_err2str(ret));
        return ret;
    }

    return 0;
}

int XExporter::closeOutFile() {
    if (!mFormatCtx) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] Output File has not open!\n");
        return -1;
    }

    if (mEncodeAudioTid) {
        mEncodeAudioTid->join();
    }

    if (mEncodeVideoTid) {
        mFrameQueue->signal();
        mEncodeVideoTid->join();
    }

    int ret = av_write_trailer(mFormatCtx.get());
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] av_write_trailer failed: %s\n", av_err2str(ret));
        return ret;
    }

    if (!mDisableAudio) {
        if (mAudioCodecCtx) {
            mAudioCodecCtx.reset();
        }
        if (mSwrContext) {
            mSwrContext.reset();
        }
    }

    if (!mDisableVideo) {
        if (mVideoCodecCtx) {
            mVideoCodecCtx.reset();
        }
        if (mSwsContext) {
            mSwsContext.reset();
        }
    }

    ret = avio_closep(&mFormatCtx->pb);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_FATAL, "[XExporter] ");
        return ret;
    }

    mFormatCtx.reset();

    return 0;
}

int XExporter::encodeVideoFrame() {
    int ret = AVERROR(EAGAIN);
    for (;;) {
        // receive packet
        do {
            auto pkt = std::make_unique<Packet>();
            ret = avcodec_receive_packet(mVideoCodecCtx.get(), pkt->avpkt);
            if (ret >= 0) {
                mReceivePacketCount++;
                av_packet_rescale_ts(pkt->avpkt, mVideoCodecCtx->time_base,mFormatCtx->streams[mVideoIndex]->time_base);
                pkt->avpkt->stream_index = mVideoIndex;
                dumpPacket(mFormatCtx.get(), pkt->avpkt);
                int tempRet = av_interleaved_write_frame(mFormatCtx.get(), pkt->avpkt);
                if (tempRet < 0) {
                    av_log(nullptr, AV_LOG_FATAL, "[XExporter] av_interleaved_write_frame failed: %s\n", av_err2str(ret));
                    return tempRet;
                }
                return 1;
            }

            if (ret == AVERROR_EOF) {
                avcodec_flush_buffers(mVideoCodecCtx.get());
                return ret;
            }
        } while (ret != AVERROR(EAGAIN));

        // get frame
        auto frame = mFrameQueue->get();
        if (!frame) {
            return -1;
        }
        frame->avframe->pts = mSendFrameCount;

        // send frame
        ret = avcodec_send_frame(mVideoCodecCtx.get(), frame->avframe);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            return ret;
        }
        mSendFrameCount++;
    }
    return 0;
}

void XExporter::debug() {
    av_log(nullptr, AV_LOG_INFO, "[XExporter] sendFrame: %d, receivePacket: %d\n", mSendFrameCount, mReceivePacketCount);
}
