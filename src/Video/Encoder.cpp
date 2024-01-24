#include "Encoder.h"
#include "Internal.h"
#include "MediaContainer.h"

#include <glob.h>

#include <iostream>

using namespace std;
namespace fs = std::filesystem;

class StillImageEncoder : public IEncoder
{
private:
    bool isOpen = false;
    OutMediaContainer outContainer;
    std::unique_ptr<InMediaContainer> audioInContainer;

public:
    StillImageEncoder(fs::path path, int64_t bitRate, int32_t width, int32_t height, FPS baseFps)
        : outContainer(path)
    {      
         if(!outContainer.IsVideoSupported())
             ERR_OUT("Video not supported in " << outContainer.GetFileName());

        outContainer.AllocVideoStream(bitRate, width, height, { baseFps.numerator, baseFps.denominator });
    }

    void UseAudioFile(const char* audioPath)
    {
        if(!outContainer.IsAudioSupported())
             ERR_OUT("Audio not supported in " << outContainer.GetFileName());

        audioInContainer = std::unique_ptr<InMediaContainer>(new InMediaContainer(audioPath));
        auto audioStream = audioInContainer->GetAudioStream();
        outContainer.AllocAudioStream(audioStream);
    }

    void EncodeImagesFittingPattern(const char* pattern, int32_t frameDuration)
    {
        if(!isOpen)
        {
            outContainer.Open();
            isOpen = true;
        }

        glob_t globResult = {0};
        if(glob(pattern, GLOB_TILDE, nullptr, &globResult) != 0)
        {
            globfree(&globResult);
            return;
        }

        for(size_t i = 0; i < globResult.gl_pathc; i++)
        {
            InMediaContainer pngInContainer(globResult.gl_pathv[i]);
            auto imageStream = pngInContainer.GetVideoStream();
            auto imageFrame = imageStream->GetNextFrame();
            outContainer.AddVideoFrame(imageFrame, frameDuration);
        }

        globfree(&globResult);
    }

    void Close() { outContainer.Close(); }

    virtual ~StillImageEncoder() = default;
};

IEncoder* AllocEncoder(fs::path path, int64_t bitRate, int32_t width, int32_t height, FPS baseFps)
{
    return new StillImageEncoder(path, bitRate, width, height, baseFps);
}