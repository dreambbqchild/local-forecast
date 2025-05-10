cmake_minimum_required(VERSION 3.18)
project(local-forecast VERSION 1.0.0)
set (CMAKE_CXX_STANDARD 20)

include(ExternalProject)

set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/forecast-repo)

ExternalProject_Add(
    rust_forecast_repo
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build COMMAND cargo build --release
    BINARY_DIR "${CMAKE_SOURCE_DIR}/forecast-repo"
    INSTALL_COMMAND ""
    LOG_BUILD ON)

include_directories(src)
include_directories(/usr/local/include)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(OpenMP_CXX_FLAG "-Xclang -fopenmp")
    set(OpenMP_CXX_INCLUDE_DIR /opt/homebrew/opt/libomp/include)
    set(OpenMP_CXX_LIB_NAMES libomp)
    set(OpenMP_C_FLAG "-Xclang -fopenmp")
    set(OpenMP_C_INCLUDE_DIR /opt/homebrew/opt/libomp/include)
    set(OpenMP_C_LIB_NAMES libomp)
    set(OpenMP_libomp_LIBRARY /opt/homebrew/opt/libomp/lib/libomp.dylib)

    include_directories(/opt/homebrew/include)

    link_directories(/opt/homebrew/lib)
endif()

link_directories(/usr/local/lib)

add_library(localforecast SHARED
    src/Astronomy/Astronomy.cpp    
    src/Data/ForecastJsonExtension.cpp
    src/Data/SelectedRegion.cpp
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
    src/Grib/GribData.cpp
    src/Grib/GribDownloader.cpp
    src/Grib/GribReader.cpp
    src/LocalForecastLib.cpp
    src/Text/SummaryForecast.cpp
    src/Video/Encoder.cpp
    src/Video/AudioStream.cpp
    src/Video/MediaContainer.cpp
    src/Video/StreamBase.cpp
    src/Video/VideoStream.cpp
    src/Wx.cpp     
)

add_dependencies(localforecast rust_forecast_repo)

find_package(OpenMP)

set_property (TARGET localforecast APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
target_link_libraries(localforecast
    debug "${CMAKE_SOURCE_DIR}/forecast-repo/target/debug/libforecast_repo.a"
    optimized "${CMAKE_SOURCE_DIR}/forecast-repo/target/release/libforecast_repo.a"
    avcodec
    avformat
    curl
    eccodes
    GeographicLib
    jsoncpp
    OpenMP::OpenMP_CXX
    sunset
    swscale
    "-framework AppKit"  "-framework CoreGraphics" "-framework ImageIO" "-framework UniformTypeIdentifiers")

add_executable(local-forecast 
    src/main.cpp 
)

target_link_libraries(local-forecast localforecast)