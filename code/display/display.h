#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "abstractModule.h"

class Display : public BaseModule
{
public:
    Display(ModuleParams params);
    ~Display();
 
    void push_frame(const cv::Mat& frame);   // 接收帧并将其放入队列
    
protected:
    void notify(string msg) override;
    void update(Publisher* publisher, string msg) override;
    

private:
    // 显示帧
    cv::Mat disp;     

    // LCD显示线程
    void display_on_lcd();

    // 线程管理与显示状态
    std::thread display_thread;
    bool display_flag = true; 
    bool pause_flag = false;
    bool quit_flag = false;

    // 互斥锁和条件变量
    std::queue<cv::Mat> queue;
    std::mutex mutex;
    std::condition_variable cond;

    // 帧缓冲参数
    int disp_width = 0;
    int disp_height = 0;
    int bit_depth = 0;
};

#endif