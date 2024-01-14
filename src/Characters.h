#ifndef CHARACTERS_H
#define CHARACTERS_H

#include <filesystem>

namespace Emoji {
    const char * const cityscape = "cityscape",
    * const cloud_with_lightning_and_rain = "cloud-with-lightning-and-rain",
    * const cloud_with_lightning = "cloud-with-lightning",
    * const cloud_with_rain = "cloud-with-rain",
    * const cloud_with_snow = "cloud-with-snow",
    * const cloud = "cloud",
    * const droplet = "droplet",
    * const fog = "fog",
    * const foggy = "foggy",
    * const high_voltage = "high-voltage",
    * const ice = "ice",
    * const night_with_stars = "night-with-stars",
    * const red_question_mark = "red-question-mark",
    * const snowflake = "snowflake",
    * const sun_behind_large_cloud = "sun-behind-large-cloud",
    * const sun_behind_small_cloud = "sun-behind-small-cloud",
    * const sun_with_face = "sun-with-face",
    * const sun = "sun",
    * const sunset = "sunset",
    * const thermometer = "thermometer";
    
    const char* const All[] = {
        cityscape,
        cloud_with_lightning_and_rain,
        cloud_with_lightning,
        cloud_with_rain,
        cloud_with_snow,
        cloud,
        droplet,
        fog,
        foggy,
        high_voltage,
        ice,
        night_with_stars,
        red_question_mark,
        snowflake,
        sun_behind_large_cloud,
        sun_behind_small_cloud,
        sun_with_face,
        sun,
        sunset,
        thermometer,
    };

    const size_t EmojiCount = sizeof(All) / sizeof(const char*);

    inline std::string Path(const char* emojiName) { return std::string("emoji") + std::filesystem::path::preferred_separator + std::string(emojiName) + ".png"; }
    inline const char* GetSkyEmoji(int32_t totalCoudCover, bool lightning, std::string precipType, double precipRate)
    {   
        if(!precipRate && lightning)
            return Emoji::cloud_with_lightning;
        else if(precipRate && lightning)
            return Emoji::cloud_with_lightning_and_rain;
        else if(precipType.length()) {
            if(precipType == "ice")
                return Emoji::ice;
            else if(precipType == "rain")
                return Emoji::cloud_with_rain;
            else if(precipType == "snow")
                return Emoji::cloud_with_snow;

            return Emoji::red_question_mark;
        }
        else if(totalCoudCover < 5)
            return Emoji::sun_with_face;
        else if(totalCoudCover < 33)
            return Emoji::sun;
        else if(totalCoudCover < 66)
            return Emoji::sun_behind_small_cloud;
        else if(totalCoudCover < 95)
            return Emoji::sun_behind_large_cloud;

        return Emoji::cloud;
    }

    inline std::string GetSkyEmojiPath(int32_t totalCoudCover, bool lightning, std::string precipType, double precipRate)
    {
        return Path(GetSkyEmoji(totalCoudCover, lightning, precipType, precipRate));
    }
};

#define GetSkyEmojiPathFromWxJson(wx, i) Emoji::GetSkyEmojiPath(wx["totalCloudCover"][i].asInt(), wx["lightning"][i].asInt() != 0, wx["precipType"][i].asString(), wx["precipRate"][i].asDouble())

namespace Unicode {
    const char* const arrows[] = {"↓", "↙", "←", "↖", "↑", "↗", "→", "↘"};

    inline const char* GetWindArrow(int deg) { return arrows[((deg + 22) % 360) / 45]; }
}

#endif