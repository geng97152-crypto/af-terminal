#include "videoRTSP.h"
#include "log.h"        // log

extern "C" {
    #include "rtsp.h"
}
// osd
#include "luckfox_osd.h"
#include "luckfox_osd_draw.h"
VideoRTSP::VideoRTSP(ModuleParams params):VideoBase(params)
{
    video_width  = 2304;
    video_height = 1296;
    pipeId = 0;            // pipeId
    viChannelId = 0;       // VI channel id
    vencChannelId = 0;     // VENC channel id

    quit_flag = false;
    
    // 初始化特定于 RTSP 的资源
    rkaiq_init();
    rkmpi_sys_init();
    vi_dev_init();
    rtsp_init();
    
    LOG_DEBUG("************************VideoRTSP init success\n");
    video_thread = new std::thread(&VideoRTSP::video_thread_func, this);
}

VideoRTSP::~VideoRTSP()
{
    quit_flag = true;

    if(video_thread->joinable()) {
        video_thread->join();
    }
    delete video_thread;

    // 工作线程已停止 VENC/VI 后，再释放共享的底层资源。
    rtsp_release();
    vi_dev_deinit();
    rkmpi_sys_deinit();
    rkaiq_deinit();
}
void VideoRTSP::notify(std::string msg)
{

}
void VideoRTSP::update(Publisher* publisher, string msg)
{

}

void VideoRTSP::videoCapture() 
{
    
}

void VideoRTSP::videoEncode() 
{

}

void VideoRTSP::videoRtspTransmit() 
{

}

void VideoRTSP::video_thread_func()
{
    LOG_DEBUG("************************VideoRTSP thread started success\n");

    stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));

    vi_chn_init(viChannelId, video_width, video_height);
    venc_init(pipeId, video_width, video_height, RK_VIDEO_ID_AVC, RK_FMT_YUV420SP);

    MPP_CHN_S vi_chn, venc_chn;
    bind_vi_to_venc(pipeId, &vi_chn, &venc_chn);

    rkipc_osd_init();
    
    while (!quit_flag)
    {
        // 获取编码后的帧，发送到 RTSP 服务器
        if (rtsp_send_frame_h264(vencChannelId, &stFrame) == RK_SUCCESS) {
            // 只有成功获取到编码帧后才能释放。
            venc_release_frame(vencChannelId, &stFrame);
        }
    }

    rkipc_osd_deinit();

    usleep(500 * 1000);
    unbind_vi_to_venc(pipeId, &vi_chn, &venc_chn);
    venc_deinit(vencChannelId);
    vi_chn_deinit(pipeId, viChannelId);
    free(stFrame.pstPack);
    stFrame.pstPack = nullptr;
    LOG_DEBUG("******************************VideoRTSP thread release success");
}
