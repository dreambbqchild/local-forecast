#pragma once 
#include "StreamBase.h"
#include <unordered_map>

class InVideoStream : public InStreamBase
{
public:
    InVideoStream(AVFormatContext* formatContext, AVCodecID codecId, int32_t streamIndex) : InStreamBase(formatContext, codecId, streamIndex) { OpenCodec(); }
    virtual ~InVideoStream() = default;
};

class OutVideoStream : public OutStreamBase
{
private:    
    const int64_t bitRate;
    const int32_t width, height;
    const AVRational timeBase;
    const AVPixelFormat pxFormat;
    int32_t nextPts = 0;

    std::unordered_map<AVPixelFormat, SwsContext*> converterMap;
    void ConfigureCodecContext(AVCodecContext *codecContext);

    void PrepareFrame(AVFrame* frame, AVPixelFormat pxFormat);
    SwsContext* GetConverter(AVPixelFormat srcPxFormat);

public:
    OutVideoStream(AVFormatContext* formatContext, AVCodecID codecId, const char* fileName, int64_t bitrate, int32_t width, int32_t height, AVRational timeBase, AVPixelFormat pxFormat = AV_PIX_FMT_YUV420P);
    void AddFrame(const AVFrame* srcFrame, int frameDuration = 1);
    virtual ~OutVideoStream();
};