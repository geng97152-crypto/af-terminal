#include "display.h"
#include "log.h"

extern "C" {
    #include "framebuffer.h"
}

Display::Display(const ModuleParams params): BaseModule(params)
{
    if(init_framebuffer() < 0) {
        throw std::runtime_error("error initializing framebuffer /dev/fb0");
    }
    // 获取显示参数
    disp_width = get_framebuffer_width();
    disp_height = get_framebuffer_height();
    bit_depth = get_framebuffer_pixel_size();
    printf("Screen width = %d, Screen height = %d, Pixel_size = %d\n",disp_width, disp_height, bit_depth);
    
	// 获取帧缓冲区位深度大小 设置颜色
	if( bit_depth == 4 )			//ARGB8888
		disp = cv::Mat(disp_height, disp_width, CV_8UC4);	// BGRX
    else if ( bit_depth == 2 ) 	//RGB565
		disp = cv::Mat(disp_height, disp_width, CV_16UC1); 	// GRAY

    // 启动显示线程
    display_thread = std::thread(&Display::display_on_lcd, this);
}

Display::~Display() 
{
    // 设置退出标志，并唤醒所有等待的线程
    {
        // 作用域自动加锁，超出作用域自动解锁
        std::lock_guard<std::mutex> lock(mutex);
        quit_flag = true;
    }
    cond.notify_all();
    
    // 等待线程结束
    if (display_thread.joinable()) {
        display_thread.join();
    }

    /**
     * @brief 释放 Framebuffer 资源，取消内存映射。
     */
    framebuffer_deinit();

    std::cout << "display quit success";

}

void Display::notify(string msg)
{

}
void Display::update(Publisher* publisher, string msg)
{
    
}



 // 接收帧并将其放入队列
void Display::push_frame(const cv::Mat& frame)
{
    if (quit_flag) {
        return;
    }


    {
        std::lock_guard<std::mutex> lock(mutex); 
        if (queue.size() < 3)
            queue.push(frame.clone());
    }
    cond.notify_one();  // 通知显示
}

void Display::display_on_lcd(void)
{
    while (!quit_flag)
    {
        // 等待条件变量 等主线程完成后 LCD显示图像
		std::unique_lock<std::mutex> lock(mutex);
        //等待队列有数据
        cond.wait(lock, [this](){
            return !queue.empty() || quit_flag; 
        });

        if (quit_flag && queue.empty()) {
            break;
        }

        // 取出一帧
        cv::Mat frame = queue.front();
        queue.pop();
        lock.unlock(); 			// 释放锁 允许其他线程推送

		cv::Mat resized_frame;
		cv::resize(frame, resized_frame, cv::Size(disp_width, disp_height));
        cv::cvtColor(resized_frame, disp, cv::COLOR_BGR2BGR565);
		framebuffer_set_frame((uint16_t*)disp.data, disp_width, disp_height);
    }
}
