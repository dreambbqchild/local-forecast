#pragma once

#include <iostream>

#define ERR_OUT(streamCommands) {\
    std::cerr << streamCommands << std::endl; \
    exit(1);\
}