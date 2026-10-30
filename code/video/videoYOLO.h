#ifndef VIDEOYOLO_H
#define VIDEOYOLO_H

#include "videoBase.h"
#include "signal_slot.h"
#include "lcd_detection.h"
#include <vector>
#include <atomic>

class VideoYOLO : public VideoBase
{
public:
    VideoYOLO(ModuleParams params);
    ~VideoYOLO();

    Signal<int, int> signal_adjust_pantilt;
    // Publish a complete detection snapshot for the LCD thread.
    Signal<std::vector<LcdDetection>> detection_signal;
protected:
    // 观察者模式数据通信接口
    void notify(string msg) override;
    void update(Publisher* publisher, string msg) override;

    // 摄像头采集
    void videoCapture() override;
    void videoEncode() override;
    void videoRtspTransmit() override;
    
private:
    int pipeId;
    int viChannelId;
    int vencChannelId;
    int video_width;
    int video_height;
    int rgn_video_width;
    int rgn_video_height;


    std::thread *video_thread;
    void video_thread_func();
    std::atomic<bool> quit_flag{false};

    VIDEO_FRAME_INFO_S stViFrame;


};





#endif
