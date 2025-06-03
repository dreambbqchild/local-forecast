#include "MediaContainer.h"

OutMediaContainer::OutMediaContainer(const std::string& fileName)
    : fileName(fileName)
{
    avformat_alloc_output_context2(&formatContext, nullptr, nullptr, fileName.c_str());
    if (!formatContext)
        ERR_OUT("Unable to allocate video context for " << fileName)
}

void OutMediaContainer::Open()
{
    int32_t ret = 0;
    if (!(formatContext->flags & AVFMT_NOFILE)) {
        ret = avio_open(&formatContext->pb, fileName.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0)
            ERR_OUT("Could not open" << fileName << " " << av_err2str(ret))
    }
 
    /* Write the stream header, if any. */
    ret = avformat_write_header(formatContext, nullptr);
    if (ret < 0)
        ERR_OUT("Could not write header. " << av_err2str(ret));

    isOpen = true;
}

void OutMediaContainer::AllocVideoStream(int64_t bitrate, int32_t width, int32_t height, AVRational timeBase, AVPixelFormat pxFormat)
{
    if(videoStream == nullptr)
        videoStream = std::unique_ptr<OutVideoStream>(new OutVideoStream(formatContext, formatContext->oformat->video_codec, fileName.c_str(), bitrate, width, height, timeBase, pxFormat));
}

void OutMediaContainer::AllocAudioStream(std::shared_ptr<InAudioStream> inAudioStream)
{
    if(audioStream != nullptr)
        return;

    this->inAudioStream = inAudioStream;
    audioStream = std::unique_ptr<OutAudioStream>(new OutAudioStream(formatContext, formatContext->oformat->audio_codec, inAudioStream.get()));
    nextAudioFrame = inAudioStream->GetNextFrame();
}

void OutMediaContainer::AlignAudioWithVideo()
{
    if(!audioStream)
        return;

    while(videoStream->IsAheadRelativeTo(audioStream.get()))
    {
        audioStream->AddFrame(nextAudioFrame);
        nextAudioFrame = inAudioStream->GetNextFrame();
    }
}

void OutMediaContainer::Close()
{
    videoStream->Flush();
    if(audioStream)
    {
        AlignAudioWithVideo();    
        audioStream->Flush();
    }

    av_write_trailer(formatContext);
    avio_closep(&formatContext->pb);

    isOpen = false;
}

//--- InMediaContainer

int32_t FindStreamOfMediaType(AVFormatContext* formatContext, AVMediaType mediaType)
{
    auto streamIndex = av_find_best_stream(formatContext, mediaType, -1, -1, nullptr, 0);
    if(streamIndex >= 0)
        return streamIndex;
    
    ERR_OUT("Unable to find stream of type " <<  av_get_media_type_string(mediaType))
}

template <class TInStream>
std::shared_ptr<TInStream> GetStream(AVFormatContext* formatContext, AVMediaType mediaType)
{
    auto index = FindStreamOfMediaType(formatContext, mediaType);
    auto codecId = formatContext->streams[index]->codecpar->codec_id;
    return std::shared_ptr<TInStream>(new TInStream(formatContext, codecId, index));
}

InMediaContainer::InMediaContainer(const std::string& fileName)
{
    if(avformat_open_input(&formatContext, fileName.c_str(), nullptr, nullptr) < 0)
        ERR_OUT("Unable to open context for " << fileName)    

    if(avformat_find_stream_info(formatContext, nullptr) < 0)
        ERR_OUT("Unable to find stream info for " << fileName)
}

std::shared_ptr<InVideoStream> InMediaContainer::GetVideoStream()
{
    if(videoStream == nullptr)
        videoStream = GetStream<InVideoStream>(formatContext, AVMediaType::AVMEDIA_TYPE_VIDEO);

    return videoStream;
}

std::shared_ptr<InAudioStream> InMediaContainer::GetAudioStream()
{
    if(audioStream == nullptr)
        audioStream = GetStream<InAudioStream>(formatContext, AVMediaType::AVMEDIA_TYPE_AUDIO);

    return audioStream;
}