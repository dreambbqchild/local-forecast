#pragma once
#include "StreamBase.h"

class InAudioStream : public InStreamBase
{
public:
    InAudioStream(AVFormatContext* formatContext, AVCodecID codecId, int32_t streamIndex) : InStreamBase(formatContext, codecId, streamIndex) { OpenCodec(); }
    virtual ~InAudioStream() = default;
};

class OutAudioStream : public OutStreamBase
{
private:
    void PrepareFrame(const AVFrame* srcFrame);

public:
    OutAudioStream(AVFormatContext* formatContext, AVCodecID codecId, InAudioStream* inAudioStream);
    void AddFrame(const AVFrame* srcFrame);
};