#pragma once
#include "abstractModule.h"
#include "log.h"
#include "param.h"
#include <mutex>
#include <string>
extern "C" {
#include "pwm.h"
}

// The public interface is shared by automatic tracking and TCP PTZ commands.
class Pantilt : public BaseModule {
public:
    explicit Pantilt(ModuleParams params);
    ~Pantilt();
    void up(int step);
    void down(int step);
    void left(int step);
    void right(int step);
    void reset();
    void onAjustPantilt(int delta_pan, int delta_tilt);
protected:
    void notify(string msg) override;
    void update(Publisher* publisher, string msg) override;
private:
    struct Axis {
        explicit Axis(PWM_num number) : pwm(number) {}
        PWM_num pwm;
        int angle = 0;
        int minimum = -90;
        int maximum = 90;
        int direction = 1;
        int home = 0;
        int center_us = 1500;
        int minimum_us = 1000;
        int maximum_us = 2000;
        bool ready = false;
        bool enabled = false;
        bool inversed = false;
        bool active_low = false;
        std::string polarity;
    };
    void initialize(Axis& axis, const char* name);
    bool apply(Axis& axis, int angle);
    void move(int pan_step, int tilt_step);
    void shutdown(Axis& axis);
    Axis pan_{PWM_PAN};
    Axis tilt_{PWM_TILT};
    std::mutex mutex_;
};
