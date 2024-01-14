#include "StreamBase.h"

#ifdef __llvm__
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif

#define INBUF_SIZE 4096

StreamBase::StreamBase(const AVCodec *codec, AVCodecID codecId, AVFormatContext* formatContext)
    : codec(codec), formatContext(formatContext)
{
    SAFE_SET(codecContext, avcodec_alloc_context3(codec), "Couldn't allocate encoding context")
    SAFE_SET(frame, av_frame_alloc(), "Unable to allocate frame")
}

void StreamBase::OpenCodec()
{
    auto ret = avcodec_open2(codecContext, codec, nullptr);
    if(ret < 0)
        ERR_OUT("Unable to open codec: " << av_err2str(ret))
}

StreamBase::~StreamBase()
{    
    if(frame)
        av_frame_free(&frame);

    if(codecContext)
        avcodec_free_context(&codecContext);        
}

//--- InStreamBase

InStreamBase::InStreamBase(AVFormatContext* formatContext, AVCodecID codecId, int32_t streamIndex)
    : StreamBase(avcodec_find_decoder(codecId), codecId, formatContext), streamIndex(streamIndex)
{
    int32_t ret = 0;
    if ((ret = avcodec_parameters_to_context(codecContext, formatContext->streams[streamIndex]->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to decoder context");
    }
}

InStreamBase::DecodeResult InStreamBase::Decode(const AVPacket* packet)
{
	auto ret = avcodec_send_packet(codecContext, packet);
    if(ret < 0)
        ERR_OUT("Error Sending Packet: " << av_err2str(ret))

	ret = avcodec_receive_frame(codecContext, frame);
    if(ret == 0)
        return DecodeResult::Success;

    if(ret == AVERROR(EAGAIN))
        return DecodeResult::Again;
    else if(ret == AVERROR_EOF)
        return DecodeResult::End;

    ERR_OUT("Error Receiving Frame: " << av_err2str(ret))
}

const AVFrame* InStreamBase::GetNextFrame()
{   
    AVPacket packet = {0};
    auto awaitingFrame = DecodeResult::Again;

    while(awaitingFrame == DecodeResult::Again)
    {
        if(av_read_frame(formatContext, &packet) < 0)
            return nullptr;
        
        if(packet.stream_index == streamIndex) {
            awaitingFrame = Decode(&packet);
        }
        
        av_packet_unref(&packet);
    }

    if(awaitingFrame == DecodeResult::Success)
        return frame;

    return nullptr;
}

//--- OutStreamBase

OutStreamBase::OutStreamBase(AVFormatContext* formatContext, AVCodecID codecId)
    : StreamBase(avcodec_find_encoder(codecId), codecId, formatContext)
{
    SAFE_SET(stream, avformat_new_stream(formatContext, nullptr), "Unable to allocate stream.")
    stream->id = formatContext->nb_streams - 1;

    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

void OutStreamBase::FinishFrame(AVFrame* frame)
{
    // send the frame to the encoder
    int32_t ret = avcodec_send_frame(codecContext, frame);
    if (ret < 0) 
        ERR_OUT("Error Sending frame to encoder" << av_err2str(ret))
    
    AVPacket packet = {0};
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext, &packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0)
            ERR_OUT("Error encoding frame " << av_err2str(ret))
 
        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(&packet, codecContext->time_base, stream->time_base);
        packet.stream_index = stream->index;
 
        ret = av_interleaved_write_frame(formatContext, &packet);
        if (ret < 0)
            ERR_OUT("Error writing output packet " << av_err2str(ret))

        wroteSomething = true;
    }
}

bool OutStreamBase::IsAheadRelativeTo(OutStreamBase* otherStream)
{
    if(!wroteSomething && !otherStream->wroteSomething)
        return false;

    if(wroteSomething && !otherStream->wroteSomething)
        return true;

    return av_compare_ts(frame->pts, codecContext->time_base, otherStream->frame->pts, otherStream->codecContext->time_base) >= 0;
}