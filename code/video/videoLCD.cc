#include "videoLCD.h"
#include <algorithm>
#include <cstdio>

VideoLCD::VideoLCD(ModuleParams params):VideoBase(params)
{
    pipeId = 0;
    viChannelId = 1;
    vencChannelId = 1;
    video_width = 720;
    video_height = 480;

    quit_flag = false;
    last_detection_update = std::chrono::steady_clock::now();
    video_thread = new std::thread(&VideoLCD::video_thread_func, this);
}

VideoLCD::~VideoLCD()
{
    quit_flag = true;
    if(video_thread->joinable())
        video_thread->join();
    delete video_thread;
    LOG_DEBUG("******************************VideoLCD exit\n");
}



void VideoLCD::notify(std::string msg) 
{

}

void VideoLCD::update(Publisher* publisher, string msg)
{

}


void VideoLCD::videoCapture()
{

}
void VideoLCD::videoEncode()
{

}
    
void VideoLCD::videoRtspTransmit()
{

}

void VideoLCD::update_detections(const std::vector<LcdDetection>& detections)
{
    std::lock_guard<std::mutex> lock(detections_mutex);
    latest_detections = detections;
    last_detection_update = std::chrono::steady_clock::now();
}



void VideoLCD::video_thread_func()
{
    LOG_DEBUG("******************************VideoLCD  video_thread_func start\n");

#if FPS_SHOW
    char fps_text[16];
    float fps = 0;
    
#endif


    stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));//接收编码后的数据

    MB_BLK src_blk; // 内存块的句柄
    MB_POOL src_Pool;//内存池的句柄
    create_MB_pool(&src_blk, &src_Pool, video_width, video_height);//根据画面宽高创建内存池，并从中取得内存块。

    
    venc_frame.stVFrame.u32Width = video_width;
    venc_frame.stVFrame.u32Height = video_height;
    venc_frame.stVFrame.u32VirWidth = video_width;
    venc_frame.stVFrame.u32VirHeight = video_height;
    venc_frame.stVFrame.enPixelFormat = RK_FMT_RGB888;
    venc_frame.stVFrame.u32FrameFlag = 160;
    venc_frame.stVFrame.pMbBlk = src_blk;

    unsigned char *venc_data = (unsigned char *)RK_MPI_MB_Handle2VirAddr(src_blk);//取得 src_blk 对应内存块的可访问地址
    cv::Mat frame(cv::Size(video_width, video_height), CV_8UC3, venc_data);
    cv::Mat yuv420sp(video_height + video_height / 2, video_width, CV_8UC1);
    cv::Mat bgr(video_height, video_width, CV_8UC3);

    vi_chn_init(viChannelId, video_width, video_height);
    venc_init(vencChannelId, video_width, video_height, RK_VIDEO_ID_AVC, RK_FMT_RGB888);

    while (!quit_flag)
    {
        unsigned char *vi_data = (unsigned char *)vi_get_frame(
            pipeId, viChannelId, video_width, video_height, &stViFrame);//返回的图像数据地址
        if (vi_data == nullptr) {
            usleep(10 * 1000);
            continue;
        }
        yuv420sp.data = vi_data;

        bgr.data = venc_data;
        cv::cvtColor(yuv420sp, bgr, cv::COLOR_YUV420sp2BGR);
        
        // 240x320 portrait LCD: center-crop the camera frame without stretching it.
        const int lcd_width = 240;
        const int lcd_height = 320;
        const int crop_height = video_height;
        const int crop_width = crop_height * lcd_width / lcd_height; // 360
        const int crop_x = (video_width - crop_width) / 2;
        cv::Mat camera_view = frame(cv::Rect(crop_x, 0, crop_width, crop_height));
        cv::Mat dst;
        cv::resize(camera_view, dst, cv::Size(lcd_width, lcd_height),
                   0, 0, cv::INTER_AREA);

        std::vector<LcdDetection> detections;
        {
            std::lock_guard<std::mutex> lock(detections_mutex);
            const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - last_detection_update).count();
            if (age < 1500)
                detections = latest_detections;
        }

        for (const LcdDetection& det : detections) {
            // 将检测框坐标换算到 240x320 的 LCD 图像上。
            int x1 = (int)((det.left * video_width - crop_x) * lcd_width / crop_width);
            int y1 = (int)(det.top * lcd_height);
            int x2 = (int)((det.right * video_width - crop_x) * lcd_width / crop_width);
            int y2 = (int)(det.bottom * lcd_height);

            x1 = std::max(0, std::min(lcd_width - 1, x1));
            y1 = std::max(0, std::min(lcd_height - 1, y1));
            x2 = std::max(0, std::min(lcd_width - 1, x2));
            y2 = std::max(0, std::min(lcd_height - 1, y2));
            if (x2 <= x1 || y2 <= y1)
                continue;

            const cv::Scalar color(0, 255, 255); // yellow in BGR
            cv::rectangle(dst, cv::Point(x1, y1), cv::Point(x2, y2), color, 1);

            char label_text[64];
            std::snprintf(label_text, sizeof(label_text), "%s %.0f%%",
                          det.label.c_str(), det.confidence * 100.0f);
            const double font_scale = 0.32;
            const int thickness = 1;
            int baseline = 0;
            cv::Size text_size = cv::getTextSize(label_text,
                cv::FONT_HERSHEY_SIMPLEX, font_scale, thickness, &baseline);
            int text_x = std::max(0, std::min(lcd_width - text_size.width, x1));
            int text_y = y1 - 2;
            if (text_y - text_size.height < 0)
                text_y = std::min(lcd_height - 1, y1 + text_size.height + 2);
            cv::rectangle(dst,
                cv::Point(text_x, text_y - text_size.height - 1),
                cv::Point(std::min(lcd_width - 1, text_x + text_size.width + 1),
                          std::min(lcd_height - 1, text_y + baseline)),
                cv::Scalar(0, 0, 0), cv::FILLED);
            cv::putText(dst, label_text, cv::Point(text_x, text_y),
                        cv::FONT_HERSHEY_SIMPLEX, font_scale, color, thickness,
                        cv::LINE_AA);
        }//把每个检测目标的框映射到裁剪、缩小后的 LCD 画面上，再画出方框和标签。

#if FPS_SHOW
        sprintf(fps_text, "fps = %.2f", fps);
        cv::putText(dst, fps_text,
                    cv::Point(2, 11),
                    cv::FONT_HERSHEY_SIMPLEX, 0.30,
                    cv::Scalar(0, 255, 0), 1);
#endif

        // Emit only after all overlays have been drawn.
        video_frame_signal.emit(dst);

        int encode_ret = venc_encode_frame(vencChannelId, &venc_frame);
        int stream_ret = encode_ret == RK_SUCCESS
            ? rtsp_send_frame_h264(vencChannelId, &stFrame)
            : encode_ret;

#if FPS_SHOW
        RK_U64 nowUs = TEST_COMM_GetNowUs();
        fps = (float)1000000 / (float)(nowUs - venc_frame.stVFrame.u64PTS);
#endif
        if (stream_ret == RK_SUCCESS)
            venc_release_frame(vencChannelId, &stFrame);    // 释放
        vi_release_frame(pipeId, viChannelId, &stViFrame);  // 释放   
    
    }

    venc_deinit(vencChannelId);
    vi_chn_deinit(pipeId, viChannelId);
    free(stFrame.pstPack);
    destroy_MB_pool(&src_blk, &src_Pool);
    LOG_DEBUG("******************************VideoLCD  video_thread_func exit\n");
}

