#pragma once

#include "AudioStream.h"
#include "VideoStream.h"

class MediaContainerBase
{
protected:
    AVFormatContext* formatContext = nullptr;
    const AVCodec *codec;

public:
    inline const AVCodec* GetAVFCodec() { return codec; }
    inline AVFormatContext* GetAVFormatContext() { return formatContext; }
    virtual ~MediaContainerBase(){ if(formatContext) avformat_free_context(formatContext); }
};

class InMediaContainer : public MediaContainerBase
{
private:
    std::shared_ptr<InVideoStream> videoStream;
    std::shared_ptr<InAudioStream> audioStream;

public:    
    InMediaContainer(const std::string& fileName);

    std::shared_ptr<InVideoStream> GetVideoStream();
    std::shared_ptr<InAudioStream> GetAudioStream();

    virtual ~InMediaContainer() = default;
};

class OutMediaContainer : public MediaContainerBase
{
private:
    std::string fileName;
    std::unique_ptr<OutVideoStream> videoStream;
    std::unique_ptr<OutAudioStream> audioStream;
    std::shared_ptr<InAudioStream> inAudioStream;
    const AVFrame* nextAudioFrame = nullptr;

    void AlignAudioWithVideo();

public:
    OutMediaContainer(const std::string& fileName);

    inline bool IsVideoSupported(){ return formatContext->oformat->video_codec != AV_CODEC_ID_NONE; }
    inline bool IsAudioSupported(){ return formatContext->oformat->audio_codec != AV_CODEC_ID_NONE; }

    void AllocVideoStream(int64_t bitrate, int32_t width, int32_t height, AVRational timeBase, AVPixelFormat pxFormat = AV_PIX_FMT_YUV420P);
    void AllocAudioStream(std::shared_ptr<InAudioStream> inAudioStream);

    inline std::string GetFileName() { return fileName; }

    void Open();
    void AddVideoFrame(const AVFrame* frame, int32_t frameDuration = 1)
    { 
        if(!videoStream) 
            return; 

        videoStream->AddFrame(frame, frameDuration); 
        AlignAudioWithVideo(); 
    }
    void Close();

    virtual ~OutMediaContainer() = default;
};