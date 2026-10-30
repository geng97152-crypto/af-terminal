#ifndef VIDEO_LCD_H
#define VIDEO_LCD_H

#include "videoBase.h"
#include "abstractModule.h"

extern "C" {
    #include "rtsp.h"
}
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "signal_slot.h"
#include "lcd_detection.h"
#include <chrono>
#include <mutex>
#include <vector>
#define FPS_SHOW 1

class VideoLCD : public VideoBase
{
protected:
    std::thread *video_thread;
    void video_thread_func();
    
    void videoCapture() override;
    void videoEncode() override;
    void videoRtspTransmit() override;

    void notify(string msg) override;
    void update(Publisher* publisher, string msg) override;
public:
    VideoLCD(ModuleParams params);
    ~VideoLCD();
    // 发送摄像头数据帧信号
    Signal<cv::Mat> video_frame_signal;
    void update_detections(const std::vector<LcdDetection>& detections);
private:
    bool quit_flag;
    VIDEO_FRAME_INFO_S stViFrame;       // 视频帧信息结构体
    VENC_STREAM_S stFrame;              // 视频帧流结构体
    VIDEO_FRAME_INFO_S venc_frame;      // 编码帧结构体

    std::mutex detections_mutex;
    std::vector<LcdDetection> latest_detections;
    std::chrono::steady_clock::time_point last_detection_update;

};

#endif
