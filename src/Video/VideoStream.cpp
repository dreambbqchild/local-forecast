#include "VideoStream.h"
#include <algorithm> 

OutVideoStream::OutVideoStream(AVFormatContext* formatContext, AVCodecID codecId, const char* fileName, int64_t bitRate, int32_t width, int32_t height, AVRational timeBase, AVPixelFormat pxFormat)
    : OutStreamBase(formatContext, codecId), bitRate(bitRate), width(width), height(height), timeBase(timeBase), pxFormat(pxFormat)
{
    ConfigureCodecContext(codecContext);
    OpenCodec();
    stream->time_base = timeBase;

    PrepareFrame(frame, pxFormat);

    auto ret = avcodec_parameters_from_context(stream->codecpar, codecContext);
    if (ret < 0)
        ERR_OUT("Could not copy the stream parameters. " << av_err2str(ret))
}

void OutVideoStream::ConfigureCodecContext(AVCodecContext *codecContext)
{
    codecContext->codec_id = formatContext->oformat->video_codec;

    codecContext->bit_rate = bitRate;

    codecContext->width = width;
    codecContext->height = height;

    codecContext->time_base = stream->time_base = timeBase;

    codecContext->gop_size = 12;
    codecContext->pix_fmt = pxFormat;
}

void OutVideoStream::PrepareFrame(AVFrame* targetFrame, AVPixelFormat pxFormat)
{
    targetFrame->format = pxFormat;
    targetFrame->width  = width;
    targetFrame->height = height;
 
    auto ret = av_frame_get_buffer(targetFrame, 0);
    if (ret < 0)
        ERR_OUT("Could not allocate frame buffer: " << av_err2str(ret));
}

SwsContext* OutVideoStream::GetConverter(AVPixelFormat srcPxFormat)
{
    if(converterMap[srcPxFormat])
        return converterMap[srcPxFormat];

    converterMap[srcPxFormat] = sws_getContext(width, height, srcPxFormat, width, height, pxFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if(!converterMap[srcPxFormat])
        ERR_OUT("Unable to generate converter")
    
    return converterMap[srcPxFormat];
}

void OutVideoStream::AddFrame(const AVFrame* srcFrame, int32_t frameDuration)
{
    if (av_frame_make_writable(frame) < 0)
        ERR_OUT("Could not make frame writable");

    if(srcFrame->format != pxFormat)
    {
        auto swsContext = GetConverter((AVPixelFormat)srcFrame->format);
        sws_scale(swsContext, (const uint8_t * const *) srcFrame->data, srcFrame->linesize, 0, height, frame->data, frame->linesize);
    }
    else
        av_frame_copy(frame, srcFrame);

    frame->pts = nextPts;
    nextPts += std::max(1, frameDuration);

    FinishFrame(frame);
}

OutVideoStream::~OutVideoStream()
{
    for(auto itr = converterMap.begin(); itr != converterMap.end(); itr++)
        sws_freeContext(itr->second);
}