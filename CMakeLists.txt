cmake_minimum_required(VERSION 3.0.0)
project(local-forecast VERSION 1.0.0)

add_executable(local-forecast main.cpp locations.cpp mapcolors.cpp ForecastArea.cpp ForecastImage.cpp GribDownloader.cpp GribReader.cpp Wx.cpp)

find_package(OpenMP)

target_link_libraries(local-forecast eccodes png GeographicLib jsoncpp curl OpenMP::OpenMP_CXX)
