#include "luckfox_osd_draw.h"


OsdDraw::OsdDraw()
{
    line_pixel = 2;     //0-3
    osd_rgn_draw_flag = false;
    coverHandle = 0;
    osd_rgn_draw__init();
    rgn_draw_thread_id = new std::thread(&OsdDraw::osd_rgn_draw_thread, this);
    osd_rgn_draw_flag = true;
}

OsdDraw::~OsdDraw()
{
    if (!osd_rgn_draw_flag) {
        LOG_DEBUG("osd_rgn_draw_thread not mrunni\n");
        return;
    }
    osd_rgn_draw_flag = false;
    sleep(1);
    rgn_draw_thread_id->join();
    while (!drawTasksQueue.empty()) {
        drawTasksQueue.pop();
    }
    osd_rgn_draw__deinit();
    LOG_DEBUG("osd_rgn_draw_thread exit");
}


// 创建/初始化一个覆盖区域 
// 参数: 区域句柄
/*
步骤1： 调用RK_MPI_RGN_Create 填充区域属性并创建区域。
步骤2：调用RK_MPI_RGN_AttachToChn 将画布绑定到通道特定区域上。
步骤3：调用RK_MPI_RGN_GetCanvasInfo 获取画布信息。
步骤4：将位图数据写入画布信息中。
步骤5：调用RK_MPI_RGN_UpdateCanvas 更新画布。
更新画布信息时重复步骤3 ~ 步骤5。
步骤6：不用时调用 RK_MPI_RGN_DetachFromChn 将画布从绑定通道中解绑。
步骤7：调用 RK_MPI_RGN_Destroy 销毁区域。
*/
int OsdDraw::osd_rgn_draw__init()   
{
    int ret = 0;
    RGN_ATTR_S stCoverAttr;         // 存储区域的属性
    RGN_CHN_ATTR_S stCoverChnAttr;  // 存储区域在特定通道上的属性
    // 创建覆盖区域 设置stCoverAttr属性
	memset(&stCoverAttr, 0, sizeof(stCoverAttr));
    stCoverAttr.enType = OVERLAY_RGN;   // 设置区域类型为覆盖区域
    // 覆盖区域的像素格式 每个像素占用 2 位数据，可以表示 4 种颜色
    stCoverAttr.unAttr.stOverlay.enPixelFmt = RK_FMT_2BPP;   
    stCoverAttr.unAttr.stOverlay.u32CanvasNum = 1;
    // 设置覆盖大小为线程1的大小
    stCoverAttr.unAttr.stOverlay.stSize.u32Width = 2304;
    stCoverAttr.unAttr.stOverlay.stSize.u32Height = 1296;
    ret = RK_MPI_RGN_Create(coverHandle, &stCoverAttr);
    if (ret != RK_SUCCESS) {
       	LOG_ERROR("RK_MPI_RGN_Create (%d) failed with %#x\n", coverHandle, ret);
		RK_MPI_RGN_Destroy(coverHandle);
		return RK_FAILURE;
    }
    // 设置/保存 属性
    ret = RK_MPI_RGN_SetAttr(coverHandle, &stCoverAttr);
	if (RK_SUCCESS != ret) {
		printf("RK_MPI_RGN_SetAttr (%d) failed with %#x!", coverHandle, ret);
		return RK_FAILURE;
	}

	// display cover regions to venc groups
    // 设置多媒体处理通道的属性
	stCoverChn.enModId = RK_ID_VENC;
	stCoverChn.s32DevId = 0;
    stCoverChn.s32ChnId = 0;
    // 存储区域在特定通道上的属性
	memset(&stCoverChnAttr, 0, sizeof(stCoverChnAttr));
	stCoverChnAttr.bShow = RK_TRUE;
	stCoverChnAttr.enType = COVER_RGN;
    // 设置大小
	stCoverChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 0;
	stCoverChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 0;
	stCoverChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 128;
	stCoverChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
    //设置颜色
	stCoverChnAttr.unChnAttr.stOverlayChn.u32ColorLUT[0] = 0x00FF0000;
    stCoverChnAttr.unChnAttr.stOverlayChn.u32ColorLUT[1] = 0x00FFFF00;
    // 设置句柄
	stCoverChnAttr.unChnAttr.stOverlayChn.u32Layer = coverHandle;
    ret = RK_MPI_RGN_AttachToChn(coverHandle, &stCoverChn, &stCoverChnAttr);
    if (RK_SUCCESS != ret) {
        LOG_ERROR("RK_MPI_RGN_AttachToChn (%d) failed with %#x\n", coverHandle, ret);
        return RK_FAILURE;
    }
    LOG_INFO("RK_MPI_RGN_AttachToChn to venc0 success\n");

    return ret;
}
int OsdDraw::osd_rgn_draw__deinit() 
{
    int ret = 0;
    RK_MPI_RGN_DetachFromChn(coverHandle, &stCoverChn);
    if (ret != RK_SUCCESS) {
        LOG_ERROR("RK_MPI_RGN_DetachFromChn (%d) failed with %#x\n", coverHandle, ret);
        return RK_FAILURE;
    }

    // 销毁覆盖区域
    ret = RK_MPI_RGN_Destroy(coverHandle);
    if (ret != RK_SUCCESS) {
        LOG_ERROR("RK_MPI_RGN_Destroy (%d) failed with %#x\n", coverHandle, ret);
        return RK_FAILURE;
    }

    // 停止绘制线程
    osd_rgn_draw_flag = false;
    // 等待线程结束
    if (rgn_draw_thread_id->joinable()) {
        rgn_draw_thread_id->join();
    }
    LOG_DEBUG("rgn_draw_thread success and xit");
    return ret;
}

// 添加一批需要绘制的任务的参数打包成一个任务并添加到到队列中
int OsdDraw::osd_rgn_add_tasks(std::vector<DrawTaskParams>& params)
{
    if (params.size() <= 0) {
        return -1;   // 处理空任务的情况
    }
    // 获取任务数量
    int num = params.size();      
    // 创建一个新的 DrawBatchTasks 对象
    DrawBatchTasks batchTasks(num);
    batchTasks.params = params;
    // 多个小任务封装成一批任务加入队列
    drawTasksQueue.push(batchTasks);

    return 0;
}

void OsdDraw::osd_rgn_draw_thread()
{
    int ret = 0;
    // 获取区域的显示画布信息
    RGN_CANVAS_INFO_S stCanvasInfo;
	memset(&stCanvasInfo, 0, sizeof(RGN_CANVAS_INFO_S));

    LOG_DEBUG("osd_rgn_draw_thread start.........");
    // 循环绘制
    while (osd_rgn_draw_flag)
    {
        // usleep(10 * 1000);
        if (drawTasksQueue.empty())
            continue;

        // 获取区域的显示画布信息
        ret = RK_MPI_RGN_GetCanvasInfo(coverHandle, &stCanvasInfo);
		if (ret != RK_SUCCESS) {
			LOG_ERROR("RK_MPI_RGN_GetCanvasInfo failed with %#x!", ret);
			continue;
		}
        // 区域信息大小判断
        if (stCanvasInfo.stSize.u32Width % 16 != 0 || stCanvasInfo.stSize.u32Height % 16 != 0) {
            LOG_ERROR("The width or height of the canvas error, please check! 16x");
            continue;
        }
        // 将指定内存区域清零
        memset((void *)stCanvasInfo.u64VirAddr, 0, stCanvasInfo.u32VirWidth * stCanvasInfo.u32VirHeight >> 2);

        // 取出队列头一批任务
        DrawBatchTasks curTask = drawTasksQueue.front();
        drawTasksQueue.pop();
        int taskNum = curTask.num;

        // 绘制同一批任务中的多个框
        for (int i = 0; i < taskNum; i++)
        {
            int x = curTask.params[i].x / 2 * 2;
            int y = curTask.params[i].y / 2 * 2;
            int w = curTask.params[i].w / 2 * 2;
            int h = curTask.params[i].h / 2 * 2;
            // 保证参数有效
            if (w <= 0 || h <= 0)   continue;
            // 保证参数在画布内
            if (x + w + line_pixel > stCanvasInfo.stSize.u32Width) {
                w -= line_pixel;
            }
            if (y + h + line_pixel > stCanvasInfo.stSize.u32Height) {
                h -= line_pixel;
            }
            // 绘制每个矩形框
            osd_rgn_draw_rect_2BPP((RK_U8 *)stCanvasInfo.u64VirAddr, stCanvasInfo.u32VirWidth, stCanvasInfo.u32VirHeight, x, y, w, h);
        }

        // 一批任务绘制完成 全部同时更新
        ret = RK_MPI_RGN_UpdateCanvas(coverHandle);
        if (ret != RK_SUCCESS) {
			LOG_ERROR("RK_MPI_RGN_UpdateCanvas failed with %#x!", ret);
			continue;
		}
    }
    return;
}

// 绘制矩形框
RK_S32 OsdDraw::osd_rgn_draw_rect_2BPP(RK_U8 *buffer,  RK_U32 width, RK_U32 height, int rgn_x, int rgn_y, int rgn_w, int rgn_h)
{
    // 检查输入参数是否有效
    if (buffer == nullptr || width == 0 || height == 0 || rgn_w <= 0 || rgn_h <= 0) {
        return RK_FAILURE;          // 返回错误码 -1
    }
    // 检查矩形是否超出缓冲区边界
    if (rgn_x < 0 || rgn_y < 0 || (rgn_x + rgn_w) > width || (rgn_y + rgn_h) > height) {
        return RK_FAILURE;          // 返回错误码 -1
    }

    RK_U8 *ptr = buffer;
    int value = 0xff;

    // 指针偏移到起始为止
    ptr += (width * rgn_y + rgn_x) >> 2;    
    // 上边界
    for (int i = 0; i < line_pixel; i++)
    {
        memset(ptr, value, (rgn_w + 3) >> 2);   // 赋颜色值
        ptr += width >> 2;  //挪到下一行
    }

    // 左边界
    ptr = buffer + ((width * rgn_y + rgn_x) >> 2);
    for (int i = 0; i < rgn_h; i++) {
        *ptr = value;
        ptr += width >> 2;
    }

    // 右边界
    ptr = buffer + ((width * rgn_y + rgn_x + rgn_w - 1) >> 2);
    for (int i = 0; i < rgn_h; i++) {
        *ptr = value;
        ptr += width >> 2;
    }

    // 下边界
    ptr = buffer + ((width * (rgn_y + rgn_h - 1) + rgn_x) >> 2);
    for (int i = 0; i < line_pixel; i++) {
        memset(ptr, value, (rgn_w + 3) >> 2);
        ptr += width >> 2;
    }

    return RK_SUCCESS;    
}

/*
2bpp（Bits Per Pixel，每像素位数）图像具有以下特征：
1. **颜色深度**:
   - 2bpp 图像每个像素使用 2 位来表示颜色,因此，每个像素可以表示 \(2^2 = 4\) 种不同的颜色。
2. **颜色调色板**:
   - 由于每个像素只能表示 4 种颜色，通常会使用一个颜色调色板（Color Palette）来定义这 4 种颜色, 调色板中的每个颜色通常用 RGB 值表示。
3. **存储效率**:
   - 2bpp 图像的存储效率较高，因为每个像素只需要 2 位。
4. **内存访问**:
   - 由于每个字节可以存储 4 个像素（2 位/像素），在处理 2bpp 图像时，通常需要按字节访问内存，
   并在字节内进行位操作来提取或设置单个像素的值。
   - 例如，一个字节 `0b10100101` 可以表示 4 个像素，分别为 `10`、`10`、`01` 和 `01`。
6. **应用场景**:
   - 2bpp 图像常用于嵌入式系统、低功耗设备、早期计算机图形和一些特定的图像处理场景。
示例代码中的 2bpp 特征, 2bpp 图像的特征体现在以下几个方面：
1. **位操作**:
   - `ptr += (width * rgn_y + rgn_x) >> 2;`
   - 这里 `>> 2` 表示将结果右移 2 位，相当于除以 4，因为每个字节存储 4 个像素。
   - `ptr += width >> 2;` 同理，将指针移动到下一行的起始位置。

2. **内存访问**:
   - `memset(ptr, value, (rgn_w + 3) >> 2);`
   - 这里 `(rgn_w + 3) >> 2` 计算需要填充的字节数。`+ 3` 确保向上取整，`>> 2` 将结果右移 2 位，相当于除以 4。

3. **颜色值**:
   - `*ptr = rgn_color_lut_1_left_value[line_pixel - 1];`
   - `*(ptr + ((rgn_w + 3) >> 2)) = rgn_color_lut_1_right_value[line_pixel - 1];`
   - 这里使用颜色查找表（LUT）来设置像素值，每个值占用 2 位。

### 总结
2bpp 图像的特点在于其较低的颜色深度和高效的存储方式。
在处理 2bpp 图像时，需要特别注意位操作和内存访问的方式，以确保正确地读取和设置像素值。
*/