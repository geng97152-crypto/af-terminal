#include "rtsp.h"
#include <thread>
#include "rtsp_demo.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_mb.h"
#include "log.h"

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
// rtsp demo handle
rtsp_demo_handle g_rtsplive = NULL;    
// rtsp session handle
rtsp_session_handle g_rtsp_session0,g_rtsp_session1,g_rtsp_session2;   

void rtsp_init(void)
{
    // 1. 创建 rtsp 实例
	g_rtsplive = create_rtsp_demo(554);
    // 2. 创建 rtsp 接口
	g_rtsp_session0 = rtsp_new_session(g_rtsplive, "/live/0");
    g_rtsp_session1 = rtsp_new_session(g_rtsplive, "/live/1");
    g_rtsp_session2 = rtsp_new_session(g_rtsplive, "/live/2");
    // 3. 设置 rtsp 传输属性 H264
	rtsp_set_video(g_rtsp_session0, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
    rtsp_set_video(g_rtsp_session1, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
    rtsp_set_video(g_rtsp_session2, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
    // 4. 同步 rtsp 时间戳
    rtsp_sync_video_ts(g_rtsp_session0, rtsp_get_reltime(), rtsp_get_ntptime());
    rtsp_sync_video_ts(g_rtsp_session1, rtsp_get_reltime(), rtsp_get_ntptime());
	rtsp_sync_video_ts(g_rtsp_session2, rtsp_get_reltime(), rtsp_get_ntptime());
	printf("\n************************Init rtsp 0 1 2 success!\n");
}

void rtsp_release(void)
{
    // 释放会话
    if (g_rtsp_session0) {
        rtsp_del_session(g_rtsp_session0);
        g_rtsp_session0 = NULL;
    }
    if (g_rtsp_session1) {
        rtsp_del_session(g_rtsp_session1);
        g_rtsp_session1 = NULL;
    }
    if (g_rtsp_session2) {
        rtsp_del_session(g_rtsp_session2);
        g_rtsp_session2 = NULL;
    }
    // 释放RTSP句柄
    if (g_rtsplive) {
        rtsp_del_demo(g_rtsplive);
        g_rtsplive = NULL;
    }
}


int rtsp_send_frame_h264(int channelId, VENC_STREAM_S* stFrame)
{
    // rtsp			从编码器中获取已经编码好的视频流。
    RK_S32 s32Ret = RK_MPI_VENC_GetStream(channelId, stFrame, -1);
    if(s32Ret == RK_SUCCESS)
    {
        switch (channelId)
        {
            case 0:
                if(g_rtsplive && g_rtsp_session0)
                {
                    void *pData = RK_MPI_MB_Handle2VirAddr(stFrame->pstPack->pMbBlk);
                    pthread_mutex_lock(&g_mutex);
                    // 同步 rtsp 时间戳
                    rtsp_tx_video(g_rtsp_session0, (uint8_t *)pData, stFrame->pstPack->u32Len,stFrame->pstPack->u64PTS);
                    rtsp_do_event(g_rtsplive);
                    pthread_mutex_unlock(&g_mutex);
                }
            break;

            case 1:
                if (g_rtsplive && g_rtsp_session1) 
                {
                    void *pData = RK_MPI_MB_Handle2VirAddr(stFrame->pstPack->pMbBlk);
                    pthread_mutex_lock(&g_mutex);
                    rtsp_tx_video(g_rtsp_session1, (uint8_t *)pData, stFrame->pstPack->u32Len, stFrame->pstPack->u64PTS);
                    rtsp_do_event(g_rtsplive);
                    pthread_mutex_unlock(&g_mutex);
                }
            break;

            case 2:
                if (g_rtsplive && g_rtsp_session2) 
                {
                    void *pData = RK_MPI_MB_Handle2VirAddr(stFrame->pstPack->pMbBlk);
                    pthread_mutex_lock(&g_mutex);
                    rtsp_tx_video(g_rtsp_session2, (uint8_t *)pData, stFrame->pstPack->u32Len, stFrame->pstPack->u64PTS);
                    rtsp_do_event(g_rtsplive);
                    pthread_mutex_unlock(&g_mutex);
                }
            break;
        
            default:
                LOG_ERROR("rtsp_send_frame_h264 channelId error!");
                break;
        }
    }
    return s32Ret;
}

int venc_release_frame(int channelId, VENC_STREAM_S* stFrame)
{
    RK_S32 s32Ret = RK_MPI_VENC_ReleaseStream(channelId, stFrame);
    if (s32Ret != RK_SUCCESS) {
        printf("RK_MPI_VENC_ReleaseStream error\n");
        return -1;
    }
    return 0;
}
