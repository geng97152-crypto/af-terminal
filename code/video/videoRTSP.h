#ifndef VIDEORTSP_H
#define VIDEORTSP_H
#include "videoBase.h"

class VideoRTSP : public VideoBase
{
protected:
    void videoCapture() override;
    void videoEncode() override;
    void videoRtspTransmit() override;
 

public:
    VideoRTSP(ModuleParams params);
    ~VideoRTSP();


protected:
    void notify(string msg) override;
    void update(Publisher* publisher, string msg) override;

private:
    int video_width  = 2304;
    int video_height = 1296;
    int pipeId = 0;            // pipeId
    int viChannelId = 0;       // VI channel id
    int vencChannelId = 0;     // VENC channel id

    VIDEO_FRAME_INFO_S stViFrame;   
    VENC_STREAM_S stFrame;     // VENC frame
    
    bool quit_flag;
    std::thread *video_thread;
    void video_thread_func();
};

#endif
