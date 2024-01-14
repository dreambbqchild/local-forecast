#include "AudioStream.h"

OutAudioStream::OutAudioStream(AVFormatContext* formatContext, AVCodecID codecId, InAudioStream* inAudioStream)
   : OutStreamBase(formatContext, codecId)
{
    auto srcCodecContext = inAudioStream->GetCodecContext();
    codecContext->sample_fmt = srcCodecContext->sample_fmt;
    codecContext->bit_rate = srcCodecContext->bit_rate;
    codecContext->sample_rate = srcCodecContext->sample_rate;
    codecContext->time_base = srcCodecContext->time_base;    
    av_channel_layout_copy(&codecContext->ch_layout, &srcCodecContext->ch_layout);    

    stream->time_base = srcCodecContext->time_base;

    OpenCodec();

    auto ret = avcodec_parameters_from_context(stream->codecpar, codecContext);
    if (ret < 0)
        ERR_OUT("Could not copy the stream parameters. " << av_err2str(ret))

    PrepareFrame(frame);
}

void OutAudioStream::PrepareFrame(const AVFrame* srcFrame)
{
    frame->format = codecContext->sample_fmt ;
    av_channel_layout_copy(&frame->ch_layout, &codecContext->ch_layout);
    frame->sample_rate = codecContext->sample_rate;
    frame->nb_samples = codecContext->frame_size;

    if (frame->nb_samples && av_frame_get_buffer(frame, 0) < 0)
        ERR_OUT("Unable to prepare audio frame buffer");
}

void OutAudioStream::AddFrame(const AVFrame* srcFrame)
{
    if (av_frame_make_writable(frame) < 0)
        ERR_OUT("Could not make frame writable");

    av_frame_copy(frame, srcFrame);

    frame->pts = srcFrame->pts;
    FinishFrame(frame);
}