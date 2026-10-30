#pragma once
#include <thread>
#include <iostream>
extern "C" {
    #include "luckfox_video.h"
}
#include "log.h"
#include "abstractModule.h"

class VideoBase: public BaseModule
{
protected:
    int pipeId;
    int viChannelId;
    int vencChannelId;
    int video_width;
    int video_height;

    virtual void videoCapture() = 0;
    virtual void videoEncode() = 0;
    virtual void videoRtspTransmit() = 0;

public:
    VideoBase(ModuleParams params):BaseModule(params){}

    virtual  ~VideoBase() {}



};