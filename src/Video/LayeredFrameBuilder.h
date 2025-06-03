#pragma once
#include "Internal.h"

extern "C" {
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersrc.h>
    #include <libavfilter/buffersink.h>
    #include <libavutil/avutil.h>
}

class FilterGraph {
private:
    AVFilterGraph* graph = nullptr;

public:
    FilterGraph() : graph(avfilter_graph_alloc()) {
        if (!graph) {
            ERR_OUT("Failed to allocate filter graph");
        }
    }
    ~FilterGraph() {
        if(graph)
            avfilter_graph_free(&graph);
    }

    operator AVFilterGraph*() const { return graph; }
};

class FilterContext {
private:
    AVFilterContext* ctx = nullptr;

public:
    FilterContext(){}
    FilterContext(const char* filterName, const char* instanceName, const char* args, AVFilterGraph* graph) {
        const AVFilter* filter = avfilter_get_by_name(filterName);
        if (!filter)
            ERR_OUT("Filter not found: " << filterName);

        int ret = avfilter_graph_create_filter(&ctx, filter, instanceName, args, nullptr, graph);
        if (ret < 0)
            ERR_OUT("Cannot create filter: " << instanceName);
    }

    operator AVFilterContext*() const { return ctx; }
};

class LayeredFrameBuilder {
private:
    FilterGraph filterGraph;
    FilterContext bgSrc;
    FilterContext overlaySrc;
    FilterContext sink;

    AVFrame* resultFrame = nullptr;

public:
    LayeredFrameBuilder(int32_t width, int32_t height, AVRational timeBase);
    void SetBackgroundFrame(AVFrame* frame);
    void SetForegroundFrame(AVFrame* frame);
    AVFrame* GetNextCompositedFrame();
    virtual ~LayeredFrameBuilder();
};