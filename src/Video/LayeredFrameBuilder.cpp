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
    // Initialize FFmpeg's filtering library (if necessary)
    //avfilter_register_all();

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

    // For each frame in your sequence:
    // 1. Read your background frame (if needed—if static, you can reuse a single AVFrame)
    // 2. Read the PNG frame (which has semi-transparency)
    // 3. Add both frames to the filter graph:
    //    av_buffersrc_add_frame(bg_src_ctx, backgroundFrame);
    //    av_buffersrc_add_frame(png_src_ctx, pngFrame);
    // 4. Pull the filtered/output frame from sink_ctx:
    //    av_buffersink_get_frame(sink_ctx, outFrame);
    // 5. Encode outFrame into your output video
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

/*
#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/avutil.h>
}

// RAII wrapper for AVFilterGraph
class FilterGraph {
public:
    FilterGraph() : graph(avfilter_graph_alloc()) {
        if (!graph) {
            throw std::runtime_error("Failed to allocate filter graph");
        }
    }
    ~FilterGraph() {
        // Free the filter graph and its sub-components
        avfilter_graph_free(&graph);
    }
    AVFilterGraph* get() const { return graph; }
private:
    AVFilterGraph* graph = nullptr;
};

// RAII wrapper for an AVFilterContext
class FilterContext {
public:
    FilterContext(const char* filterName, const char* instanceName, const char* args, AVFilterGraph* graph) {
        const AVFilter* filter = avfilter_get_by_name(filterName);
        if (!filter) {
            throw std::runtime_error(std::string("Filter not found: ") + filterName);
        }
        int ret = avfilter_graph_create_filter(&ctx, filter, instanceName, args, nullptr, graph);
        if (ret < 0) {
            throw std::runtime_error(std::string("Cannot create filter: ") + instanceName);
        }
    }
    // We don’t free the AVFilterContext individually; it's managed by the graph.
    AVFilterContext* get() const { return ctx; }
private:
    AVFilterContext* ctx = nullptr;
};

int main() {
    // Initialize FFmpeg's filtering library (if necessary)
    avfilter_register_all();

    // Parameters for the buffers (example values)
    int width = 1280;
    int height = 720;
    int pix_fmt = AV_PIX_FMT_RGBA; // assuming input PNGs with alpha

    // Build the argument string for the buffer source
    std::string args = "video_size=" + std::to_string(width) + "x" + std::to_string(height) +
                       ":pix_fmt=" + std::to_string(pix_fmt) +
                       ":time_base=1/25:pixel_aspect=1/1";

    try {
        // Create the filter graph
        FilterGraph filterGraph;

        // Create buffer sources for the background and PNG overlay
        FilterContext bgSrc("buffer", "in_bg", args.c_str(), filterGraph.get());
        FilterContext pngSrc("buffer", "in_png", args.c_str(), filterGraph.get());

        // Create the buffer sink
        FilterContext sink("buffersink", "out", nullptr, filterGraph.get());

        // Allocate AVFilterInOut structures for the filter chain configuration.
        AVFilterInOut* outputs = avfilter_inout_alloc();
        AVFilterInOut* inputs  = avfilter_inout_alloc();
        if (!outputs || !inputs) {
            throw std::runtime_error("Could not allocate filter in/out structures");
        }

        // Set up two inputs: one for the background and one for the PNG overlay.
        outputs->name       = av_strdup("in_bg");
        outputs->filter_ctx = bgSrc.get();
        outputs->pad_idx    = 0;
        outputs->next       = avfilter_inout_alloc();
        outputs->next->name       = av_strdup("in_png");
        outputs->next->filter_ctx = pngSrc.get();
        outputs->next->pad_idx    = 0;
        outputs->next->next       = nullptr;

        // The output label in our filter chain.
        inputs->name       = av_strdup("out");
        inputs->filter_ctx = sink.get();
        inputs->pad_idx    = 0;
        inputs->next       = nullptr;

        // Note the modified filter chain: we're overlaying and then converting to YUV420P.
        const char* filter_desc = "[in_bg][in_png]overlay=0:0,format=yuv420p[out]";
        int ret = avfilter_graph_parse_ptr(filterGraph.get(), filter_desc, &inputs, &outputs, nullptr);
        if (ret < 0) {
            throw std::runtime_error("Error parsing filter graph");
        }
        ret = avfilter_graph_config(filterGraph.get(), nullptr);
        if (ret < 0) {
            throw std::runtime_error("Error configuring filter graph");
        }

        // Free the temporary AVFilterInOut structures.
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);

        std::cout << "Filter graph successfully set up." << std::endl;

        // Here you would loop over your frames:
        // 1. Push your background frame and corresponding PNG frame(s) via
        //    av_buffersrc_add_frame().
        // 2. Retrieve the output composite frame using av_buffersink_get_frame().
        // 3. Feed the composite frame to your encoder.

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return -1;
    }
    return 0;
}






// Assume these helper functions exist and return an allocated AVFrame*:
// AVFrame* getBackgroundFrame(); // Returns the static background frame.
// AVFrame* getOverlayFrame(int i); // Returns the overlay frame for iteration i.
// void processFrame(AVFrame *frame);  // For example, encode or write the frame.

for (int i = 0; i < 10; i++) {
    // If your background is static, you can load it once outside the loop.
    // For demonstration, we assume it’s available as 'backgroundFrame'.
    // You may want to clone or reference it depending on your filter graph behavior.
    AVFrame *backgroundFrame = getBackgroundFrame();
    if (!backgroundFrame) {
        throw std::runtime_error("Could not obtain background frame");
    }
    
    // Get the overlay frame for this iteration.
    AVFrame *overlayFrame = getOverlayFrame(i);
    if (!overlayFrame) {
        throw std::runtime_error("Could not obtain overlay frame");
    }
    
    // Push the background frame into the "in_bg" buffer source.
    int ret = av_buffersrc_add_frame(bgSrc.get(), backgroundFrame);
    if (ret < 0) {
        throw std::runtime_error("Error adding background frame to filter graph");
    }
    
    // Push the overlay frame into the "in_png" (overlay) buffer source.
    ret = av_buffersrc_add_frame(pngSrc.get(), overlayFrame);
    if (ret < 0) {
        throw std::runtime_error("Error adding overlay frame to filter graph");
    }
    
    // Now retrieve the composite frame from the sink.
    AVFrame *filtFrame = av_frame_alloc();
    ret = av_buffersink_get_frame(sink.get(), filtFrame);
    if (ret < 0) {
        throw std::runtime_error("Error while retrieving frame from filter graph");
    }
    
    // The filter chain (which we set up as "[in_bg][in_png]overlay=0:0,format=yuv420p[out]")
    // ensures that the output is composed over the background and is converted to YUV420P.
    // Now process (encode, write, etc.) the composite frame.
    processFrame(filtFrame);
    
    // Clean up the temporary frames.
    av_frame_free(&filtFrame);
    av_frame_free(&overlayFrame);
    
    // If your background frame is static and you push a new one   each time,
    // you might need to either reinitialize it or clone it. In some workflows,
    // you might load it once and simply reference the same frame if the filter graph
    // makes an internal copy. Check your use-case and FFmpeg’s reference counting.
}
*/