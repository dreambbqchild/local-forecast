#pragma once 
#include "Internal.h"

class StreamBase
{
private:
    const AVCodec* codec;
    
protected:
    AVCodecContext *codecContext = nullptr;
    AVFrame* frame = nullptr;
    AVFormatContext* formatContext;

    void OpenCodec();

public:
    StreamBase(const AVCodec *codec, AVCodecID codecId, AVFormatContext* formatContext);
    const AVCodecContext* GetCodecContext(){ return codecContext; }
    virtual ~StreamBase();
};

//--- InStreamBase

class InStreamBase : public StreamBase
{
private:
    enum DecodeResult 
    {
        Success,
        Again,
        End
    };

    int32_t streamIndex;

    DecodeResult Decode(const AVPacket* packet);

public:
    InStreamBase(AVFormatContext* formatContext, AVCodecID codecId, int32_t streamIndex);

    const AVFrame* GetNextFrame();

    virtual ~InStreamBase() = default;
};

//--- OutStreamBase

class OutStreamBase : public StreamBase 
{
private:
    bool wroteSomething = false;

protected:
    AVStream* stream = nullptr;

    void FinishFrame(AVFrame* frame);

public: 
    OutStreamBase(AVFormatContext* formatContext, AVCodecID codecId);
    bool IsAheadRelativeTo(OutStreamBase* otherStream);

    void Flush() { FinishFrame(nullptr); }

    virtual ~OutStreamBase() = default;
};