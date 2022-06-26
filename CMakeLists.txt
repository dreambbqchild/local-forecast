project(local-forecast)
cmake_minimum_required(VERSION 3.0)

add_executable(local-forecast main.c locations.c)

target_link_libraries(local-forecast eccodes json-c m)