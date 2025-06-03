#include "Encoder.h"
#include "Internal.h"
#include "LayeredFrameBuilder.h"
#include "MediaContainer.h"

#include <glob.h>

#include <functional>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

class BackgroundFrame
{
private:
    AVFrame* frame;
    
public:
    BackgroundFrame(const string& background)
    {
        InMediaContainer mediaContainer(background);
        auto stream = mediaContainer.GetVideoStream();
        frame = av_frame_clone(stream->GetNextFrame());
    }

    operator AVFrame*() const { return frame; }
    virtual ~BackgroundFrame() = default;
};


class Glob {
private:
    glob_t globResult = {0};

public:
    Glob(const string& pattern) {
        if(glob(pattern.c_str(), GLOB_TILDE, nullptr, &globResult) != 0)
        {
            globfree(&globResult);
            return;
        }
    }
    
    void ForEach(std::function<void (const char *)> callback) {
        for(size_t i = 0; i < globResult.gl_pathc; i++)
            callback(globResult.gl_pathv[i]);
    }
    
    virtual ~Glob() { globfree(&globResult); }
};

class StillImageEncoder : public IEncoder
{
private:
    const int32_t width, height;
    const FPS baseFps;

    OutMediaContainer outContainer;
    std::unique_ptr<InMediaContainer> audioInContainer;

public:
    StillImageEncoder(fs::path path, int64_t bitRate, int32_t width, int32_t height, FPS baseFps)
        : outContainer(path), width(width), height(height), baseFps(baseFps)
    {      
         if(!outContainer.IsVideoSupported())
            ERR_OUT("Video not supported in " << outContainer.GetFileName());

        outContainer.AllocVideoStream(bitRate, width, height, { baseFps.numerator, baseFps.denominator });
    }

    void UseAudioFile(const string& audioPath)
    {
        if(!outContainer.IsAudioSupported())
            ERR_OUT("Audio not supported in " << outContainer.GetFileName());

        audioInContainer = std::unique_ptr<InMediaContainer>(new InMediaContainer(audioPath));
        auto audioStream = audioInContainer->GetAudioStream();
        outContainer.AllocAudioStream(audioStream);
    }

    void EncodeImagesFittingPattern(const string& pattern, int32_t frameDuration)
    {
        if(!outContainer.IsOpen())
            outContainer.Open();

        Glob glob(pattern);
        glob.ForEach([&](const char * path)
        {
            InMediaContainer pngInContainer(path);
            auto imageStream = pngInContainer.GetVideoStream();
            auto imageFrame = imageStream->GetNextFrame();
            outContainer.AddVideoFrame(imageFrame, frameDuration);
        });
    }

    void EncodeImagesFittingPattern(const string& pattern, int32_t frameDuration, const string& background)
    {
        if(!outContainer.IsOpen())
            outContainer.Open();

        BackgroundFrame bgFrame(background);
        LayeredFrameBuilder layers(width, height, { baseFps.numerator, baseFps.denominator });

        Glob glob(pattern);
        glob.ForEach([&](const char * path)
        {
            InMediaContainer pngInContainer(path);
            auto imageStream = pngInContainer.GetVideoStream();
            auto imageFrame = av_frame_clone(imageStream->GetNextFrame());

            layers.SetBackgroundFrame(bgFrame);
            layers.SetForegroundFrame(imageFrame);
            auto layeredFrame = layers.GetNextCompositedFrame();

            outContainer.AddVideoFrame(layeredFrame, frameDuration);
            av_frame_free(&imageFrame);
        });
    }

    void Close() { outContainer.Close(); }

    virtual ~StillImageEncoder() = default;
};

IEncoder* AllocEncoder(fs::path path, int64_t bitRate, int32_t width, int32_t height, FPS baseFps)
{
    return new StillImageEncoder(path, bitRate, width, height, baseFps);
}