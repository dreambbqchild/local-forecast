cmake_minimum_required(VERSION 3.0.0)
project(local-forecast VERSION 1.0.0)
set (CMAKE_CXX_STANDARD 20)

include_directories(src)

link_directories(/usr/local/lib)

add_executable(local-forecast 
    src/main.cpp 
    src/Astronomy/Astronomy.cpp    
    src/Data/ForecastJsonExtension.cpp
    src/Data/StringCache.cpp
    src/DateTime.cpp 
    src/Drawing/DrawService.cpp 
    src/Drawing/DrawTextContext.mm     
    src/Drawing/ForecastImages/MapOverlay.cpp
    src/Drawing/ForecastImages/PersonalForecasts.cpp
    src/Drawing/ForecastImages/RegionalForecast.cpp 
    src/Drawing/ForecastImages/WeatherMaps.cpp
    src/Drawing/ImageCache.cpp
    src/Drawing/WxColors.cpp
    src/Geography/Geo.cpp
    src/Grib/GribDownloader.cpp
    src/Grib/GribProcessor.cpp     
    src/Grib/GribReader.cpp
    src/Locations.cpp
    src/Text/SummaryForecast.cpp
    src/Video/Encoder.cpp
    src/Video/AudioStream.cpp
    src/Video/MediaContainer.cpp
    src/Video/StreamBase.cpp
    src/Video/VideoStream.cpp
    src/Wx.cpp     
)

find_package(OpenMP)

set_property (TARGET local-forecast APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
target_link_libraries(local-forecast avcodec avformat curl eccodes GeographicLib jsoncpp OpenMP::OpenMP_CXX sunset swscale "-framework AppKit"  "-framework CoreGraphics" "-framework ImageIO" "-framework UniformTypeIdentifiers")
