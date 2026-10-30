#include "videoYOLO.h"
#include <yolov5.h>
// osd
#include "luckfox_osd.h"
#include "luckfox_osd_draw.h"
#include "person_tracker.h"
#include <chrono>

VideoYOLO::VideoYOLO(ModuleParams params):VideoBase(params)
{
    pipeId = 0;
    viChannelId = 2;
    video_width = 640;
    video_height = 640;

    rgn_video_width = 2304;
    rgn_video_height = 1296;

    quit_flag = false;
    video_thread = new std::thread(&VideoYOLO::video_thread_func, this);
}

VideoYOLO::~VideoYOLO()
{
    quit_flag = true;
    if(video_thread->joinable())
        video_thread->join();
    delete video_thread;
    LOG_DEBUG("******************************VideoLCD exit\n");
}


void VideoYOLO::notify(std::string msg)
{

}
void VideoYOLO::update(Publisher* publisher, string msg)
{

}

void VideoYOLO::videoCapture() 
{
    vi_get_frame(pipeId, viChannelId, video_width, video_height, &stViFrame);
}

void VideoYOLO::videoEncode() 
{
    // 无需编码器
    LOG_DEBUG("******************************VideoYOLO encode free\n");
}

void VideoYOLO::videoRtspTransmit() 
 {
    // 无需传输
    LOG_DEBUG("******************************VideoYOLO rtsp transmit free\n");
 }

void VideoYOLO::video_thread_func()
{
    LOG_DEBUG("************************video_thread_2 started success\n");


    PersonTrackerConfig tracker_config;
    tracker_config.enabled = rk_param_get_int("ptz:auto_track_enable", 1) != 0;
    tracker_config.min_confidence = std::max(0.01f, std::min(1.f,
        rk_param_get_int("ptz:track_min_confidence_percent", 25) / 100.f));
    tracker_config.interval_ms = std::max(80, rk_param_get_int("ptz:track_interval_ms", 200));
    tracker_config.lost_timeout_ms = std::max(tracker_config.interval_ms,
        rk_param_get_int("ptz:track_lost_timeout_ms", 800));
    tracker_config.max_step = std::max(1, std::min(5, rk_param_get_int("ptz:track_max_step_deg", 3)));
    tracker_config.dead_zone_x = std::max(0.02f, std::min(0.4f,
        rk_param_get_int("ptz:track_dead_zone_x_percent", 8) / 100.f));
    tracker_config.dead_zone_y = std::max(0.02f, std::min(0.4f,
        rk_param_get_int("ptz:track_dead_zone_y_percent", 8) / 100.f));
    const int display_source_width = std::max(1, rk_param_get_int("video.1:width", 720));
    const int display_source_height = std::max(1, rk_param_get_int("video.1:height", 480));
    tracker_config.view_width = std::min(1.f,
        (display_source_height * 240 / 320) / (float)display_source_width);
    PersonTracker tracker(tracker_config);
    LOG_INFO("Person tracker: enabled=%d interval_ms=%d max_step=%d LCD_view_width=%.3f\n",
             tracker_config.enabled, tracker_config.interval_ms,
             tracker_config.max_step, tracker_config.view_width);

    // osd
    cv::Mat yuv420sp(video_height + video_height / 2, video_width, CV_8UC1);
    cv::Mat bgr(video_height, video_width, CV_8UC3);

    vi_chn_init(viChannelId, video_width, video_height);

    int sX, sY, eX, eY;
    rknn_app_context_t rknn_app_ctx;
    object_detect_result_list od_results;
    const char *model_path = "./model/yolov5.rknn";
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    if (init_yolov5_model(model_path, &rknn_app_ctx) != 0) {
        LOG_ERROR("Failed to initialize YOLO model: %s\n", model_path);
        vi_chn_deinit(pipeId, viChannelId);
        return;
    }
    init_post_process();

    RGN_HANDLE coverHandle = 0;
    std::vector<DrawTaskParams> params;
    OsdDraw osdDraw;
    long long last_track_report_ms = 0;

    while (!quit_flag)
    {
        unsigned char *vi_data = (unsigned char *)vi_get_frame(
            pipeId, viChannelId, video_width, video_height, &stViFrame);
        if (vi_data == nullptr) {
            usleep(10 * 1000);
            continue;
        }
        yuv420sp.data = vi_data;
        cv::cvtColor(yuv420sp, bgr, cv::COLOR_YUV420sp2BGR);
        cv::resize(bgr, bgr, cv::Size(video_width, video_height), 0, 0, cv::INTER_LINEAR);

        cv::Mat letterboxImage = letterbox(bgr, video_width, video_height);
        memcpy(rknn_app_ctx.input_mems[0]->virt_addr, letterboxImage.data, MODEL_HEIGHT * MODEL_HEIGHT * 3);
        memset(&od_results, 0, sizeof(od_results));
        if (inference_yolov5_model(&rknn_app_ctx, &od_results) != 0) {
            vi_release_frame(pipeId, viChannelId, &stViFrame);
            tracker.clear();
            LOG_ERROR("YOLO inference failed; skipping tracking frame\n");
            continue;
        }

        std::vector<LcdDetection> lcd_detections;
        lcd_detections.reserve(od_results.count);
        std::vector<LcdDetection> person_detections;
        person_detections.reserve(od_results.count);



        for (int i = 0; i < od_results.count; i++)
        {
            if(od_results.count >= 1)
            {
                object_detect_result *det_result = &(od_results.results[i]);

                sX = (int)(det_result->box.left);
                sY = (int)(det_result->box.top);
                eX = (int)(det_result->box.right);
                eY = (int)(det_result->box.bottom);
                mapCoordinates(&sX, &sY);
                mapCoordinates(&eX, &eY);

                LcdDetection lcd_det;
                lcd_det.left = std::max(0.0f, std::min(1.0f, (float)sX / (float)video_width));
                lcd_det.top = std::max(0.0f, std::min(1.0f, (float)sY / (float)video_height));
                lcd_det.right = std::max(0.0f, std::min(1.0f, (float)eX / (float)video_width));
                lcd_det.bottom = std::max(0.0f, std::min(1.0f, (float)eY / (float)video_height));
                lcd_det.confidence = det_result->prop;
                lcd_det.label = coco_cls_to_name(det_result->cls_id);
                lcd_detections.push_back(lcd_det);
                if (det_result->cls_id == 0) {
                    lcd_det.label = "person";
                    person_detections.push_back(lcd_det);
                }

                sX = (int)((float)sX / (float)video_width * rgn_video_width);
                sY = (int)((float)sY / (float)video_height * rgn_video_height);
                eX = (int)((float)eX / (float)video_width * rgn_video_width);
                eY = (int)((float)eY / (float)video_height * rgn_video_height);

                DrawTaskParams task;
                task.coverHandle = coverHandle;
                task.x = sX;
                task.y = sY;
                task.w = eX - sX;
                task.h = eY - sY;
                params.push_back(task);

            }
        }

        // Emit empty snapshots too, otherwise an old box would remain on LCD.
        detection_signal.emit(lcd_detections);
        

        if (params.size() > 0)
            osdDraw.osd_rgn_add_tasks(params);
        params.clear();
        vi_release_frame(pipeId, viChannelId, &stViFrame);


        const long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const PersonTrackCommand command = tracker.update(person_detections, now);
        if (now - last_track_report_ms >= 1000) {
            const float confidence = person_detections.empty() ? 0.0f :
                person_detections.front().confidence;
            LOG_INFO("Tracker input: objects=%d person_class0=%zu first_conf=%.2f threshold=%.2f command=(%d,%d) error=(%.2f,%.2f)\n",
                     od_results.count, person_detections.size(), confidence,
                     tracker_config.min_confidence, command.pan, command.tilt,
                     command.error_x, command.error_y);
            last_track_report_ms = now;
        }
        if (command.send) {
            LOG_DEBUG("Person tracker: error=(%.3f,%.3f) step_deg=(%d,%d)\n",
                      command.error_x, command.error_y, command.pan, command.tilt);
            signal_adjust_pantilt.emit(command.pan, command.tilt);
        }
    }
    vi_chn_deinit(pipeId, viChannelId);
    release_yolov5_model(&rknn_app_ctx);
    deinit_post_process();
    LOG_DEBUG("******************************video_thread_2 release success\n");
}
