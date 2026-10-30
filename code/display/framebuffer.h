#ifndef _FRAMEBUFF_H_
#define _FRAMEBUFF_H_
#include <fcntl.h>
#include <stdio.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

// 帧缓冲设备
#define FB_DEVICE "/dev/fb0"

// RGB颜色定义
#define RGB(r, g, b)  (((r) << 16) | ((g) << 8) | (b))
#define RGB_WHITE  0xFFFFFF
#define RGB_BLACK  0x000000
#define RGB_RED    0xFF0000
#define RGB_GREEN  0x00FF00
#define RGB_BLUE   0x0000FF


// 初始化帧缓冲
int init_framebuffer(void);
// 释放帧缓冲
void framebuffer_deinit(void);

// 获取像素参数
int get_framebuffer_pixel_size(void);
int get_framebuffer_width(void);
int get_framebuffer_height(void);

// 设置显示缓冲区的像素大小  传入参数frame 映射到framebuffer
int framebuffer_set_frame(uint16_t *frame, int width, int height);
   
#endif