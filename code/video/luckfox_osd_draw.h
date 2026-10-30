#include <rk_comm_rgn.h>
#include <rk_mpi_rgn.h>
#include <cstring>
#include "log.h"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <vector>
#include <unistd.h>

// 每一个参数
struct DrawTaskParams
{
    RGN_HANDLE coverHandle; // 区域句柄
    int x, y, w, h;         // 绘制区域的坐标
};

// 每一批任务
struct DrawBatchTasks
{
    std::vector<DrawTaskParams> params;  // 绘制参数数组
    int num;                            // 绘制的数量
    // 构造函数
    DrawBatchTasks(int num) : num(num), params(num) {}
};

class OsdDraw
{
private:
    // 绘制区域参数
    RGN_HANDLE coverHandle;         // 覆盖句柄
	MPP_CHN_S stCoverChn;           // 存储多媒体处理通道的属性
    int line_pixel;                 // 线条粗细
	
    // 功能变量
    // 绘制任务队列 注意这个队列中存放的是一批一批的任务，每一批的任务中有很多小的框任务,一批一批的绘制
    std::queue<DrawBatchTasks> drawTasksQueue; 
    std::mutex  mutex_queue; 
    std::condition_variable cond_queue;                   

    // 绘制线程函数
    void osd_rgn_draw_thread();    
    std::thread *rgn_draw_thread_id;
    bool osd_rgn_draw_flag;

    //功能函数
    int osd_rgn_draw__init();
    int osd_rgn_draw__deinit();
    // 2bpp图像缓冲区中绘制一个矩形
    RK_S32 osd_rgn_draw_rect_2BPP(RK_U8 *buffer,  RK_U32 width, RK_U32 height, int rgn_x, int rgn_y, int rgn_w, int rgn_h);

public:
    OsdDraw();
    ~OsdDraw();
    // 添加绘制任务函数
    // 传入一个绘制任务参数数组
    int osd_rgn_add_tasks(std::vector<DrawTaskParams>& params);

};