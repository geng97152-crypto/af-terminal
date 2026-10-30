// video_factory.h
#ifndef VIDEO_FACTORY_H
#define VIDEO_FACTORY_H

#include "videoBase.h"
#include "abstractFactory.h"
#include "videoRTSP.h"
#include "videoLCD.h"
#include "videoYOLO.h"

class VideoRTSPFactory: public BaseFactory{
public:
    BaseModule* createModule(const ModuleParams& params = {}) const override {
        return new VideoRTSP(params);
    }
    virtual ~VideoRTSPFactory() {}
};


class VideoDisplayFactory:  public BaseFactory{
public:
    BaseModule* createModule(const ModuleParams& params = {}) const override {
        return new VideoLCD(params);
    }
    virtual ~VideoDisplayFactory() {}
};

class VideoYOLOFactory:  public BaseFactory{
public:
    BaseModule* createModule(const ModuleParams& params = {}) const override {
        return new VideoYOLO(params);
    }
    virtual ~VideoYOLOFactory() {}
};

#endif 



