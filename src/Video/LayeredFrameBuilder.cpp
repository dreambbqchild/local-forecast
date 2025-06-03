#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>
#include "LayeredFrameBuilder.h"

using namespace std;

const AVPixelFormat pngPxFormat = AV_PIX_FMT_RGBA;
const char* in_bg = "in_bg";
const char* in_overlay = "in_overlay";

LayeredFrameBuilder::LayeredFrameBuilder(int32_t width, int32_t height, AVRational timeBase)
{
    SAFE_SET(resultFrame, av_frame_alloc(), "Unable to allocate frame")

    std::string args = "video_size=" + std::to_string(width) + "x" + std::to_string(height) + ":pix_fmt=" + std::to_string(pngPxFormat) + ":time_base=" + std::to_string(timeBase.num) + "/" + std::to_string(timeBase.den) + ":pixel_aspect=1/1";

    bgSrc = FilterContext("buffer", in_bg, args.c_str(), filterGraph);
    overlaySrc = FilterContext("buffer", in_overlay, args.c_str(), filterGraph);

    sink = FilterContext("buffersink", "out", nullptr, filterGraph);

    auto outputs = avfilter_inout_alloc();
    auto inputs  = avfilter_inout_alloc();
    if (!outputs || !inputs) {
        ERR_OUT("Could not allocate filter in/out structures");
    }

    outputs->name = av_strdup(in_bg);
    outputs->filter_ctx = bgSrc;
    outputs->pad_idx = 0;
    outputs->next = avfilter_inout_alloc();
    outputs->next->name = av_strdup(in_overlay);
    outputs->next->filter_ctx = overlaySrc;
    outputs->next->pad_idx = 0;
    outputs->next->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = sink;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    const char* filter_desc = "[in_bg][in_overlay]overlay=0:0,format=yuv420p[out]";
    int ret = avfilter_graph_parse_ptr(filterGraph, filter_desc, &inputs, &outputs, nullptr);
    if (ret < 0) {
        ERR_OUT("Error parsing filter graph");
    }
    ret = avfilter_graph_config(filterGraph, nullptr);
    if (ret < 0) {
        ERR_OUT("Error configuring filter graph");
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
}

void LayeredFrameBuilder::SetBackgroundFrame(AVFrame* frame)
{
    if (av_buffersrc_add_frame(bgSrc, frame) < 0)
        ERR_OUT("Unable to add background frame");
}

void LayeredFrameBuilder::SetForegroundFrame(AVFrame* frame)
{
    if(av_buffersrc_add_frame(overlaySrc, frame) < 0)
        ERR_OUT("Unable to add foreground frame");
}

AVFrame* LayeredFrameBuilder::GetNextCompositedFrame()
{
    if(av_buffersink_get_frame(sink, resultFrame) < 0)
        ERR_OUT("Unable to get composited frame");

    return resultFrame;
}

LayeredFrameBuilder::~LayeredFrameBuilder()
{
    av_frame_free(&resultFrame);
};