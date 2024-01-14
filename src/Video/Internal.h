//All references of this file indicate that file is for internal use by this folder.
#pragma once

#include "Error.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#define SAFE_SET(var, method, errorStreamCommands) var = method;\
if(!var)\
    ERR_OUT(errorStreamCommands)