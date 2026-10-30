#include "pantilt.h"
#include <algorithm>
#include <cstring>

namespace {
const unsigned int period_ns = 20000000;
int clamp(int value, int minimum, int maximum) {
    return std::max(minimum, std::min(maximum, value));
}
}

Pantilt::Pantilt(ModuleParams params) : BaseModule(params) {
    tilt_.minimum = -60;
    initialize(pan_, "pan");
    initialize(tilt_, "tilt");
}

void Pantilt::initialize(Axis& axis, const char* name) {
    const std::string prefix = std::string("ptz:") + name;
    axis.direction = rk_param_get_int((prefix + "_direction").c_str(),
                                      1) < 0 ? -1 : 1;
    axis.minimum = clamp(rk_param_get_int((prefix + "_min_angle").c_str(), axis.minimum), -90, 89);
    axis.maximum = clamp(rk_param_get_int((prefix + "_max_angle").c_str(), 90), axis.minimum + 1, 90);
    axis.home = clamp(rk_param_get_int((std::string("ptz:preset_") + name).c_str(), 0),
                      axis.minimum, axis.maximum);
    axis.center_us = rk_param_get_int((prefix + "_center_us").c_str(), 1500);
    axis.minimum_us = rk_param_get_int((prefix + "_min_us").c_str(), 1000);
    axis.maximum_us = rk_param_get_int((prefix + "_max_us").c_str(), 2000);
    axis.active_low = rk_param_get_int((prefix + "_active_low").c_str(), 1) != 0;
    axis.polarity = rk_param_get_string((prefix + "_pwm_polarity").c_str(), "inversed");
    if ((axis.polarity != "normal" && axis.polarity != "inversed") ||
        axis.minimum_us < 500 || axis.maximum_us > 2500 ||
        axis.minimum_us >= axis.center_us || axis.center_us >= axis.maximum_us) {
        LOG_ERROR("PTZ PWM%d invalid polarity or pulse calibration\n", (int)axis.pwm);
        return;
    }
    axis.inversed = axis.polarity == "inversed";
    if (pwm_init(axis.pwm) != 0) return;
    // A newly exported PWM has period=0. Set a valid period before writing enable.
    if (pwm_set_period(axis.pwm, period_ns) != 0 ||
        pwm_enable(axis.pwm, 0) != 0 ||
        pwm_set_polarity(axis.pwm, axis.polarity.c_str()) != 0) {
        LOG_ERROR("PTZ PWM%d setup failed\n", (int)axis.pwm);
        pwm_deinit(axis.pwm);
        return;
    }
    axis.ready = true;
    axis.angle = axis.home;
    LOG_INFO("PTZ PWM%d ready: polarity=%s active=%s range=(%d,%d); waiting for command\n",
             (int)axis.pwm, axis.polarity.c_str(), axis.active_low ? "low" : "high",
             axis.minimum, axis.maximum);
}

bool Pantilt::apply(Axis& axis, int requested) {
    if (!axis.ready) return false;
    const int angle = clamp(requested, axis.minimum, axis.maximum);
    if (axis.enabled && angle == axis.angle) {
        if (requested != angle)
            LOG_DEBUG("PTZ PWM%d at angle limit=%d\n", (int)axis.pwm, angle);
        return true;
    }
    int pulse_us = axis.center_us;
    if (angle >= 0)
        pulse_us += angle * (axis.maximum_us - axis.center_us) / 90;
    else
        pulse_us += angle * (axis.center_us - axis.minimum_us) / 90;
    const unsigned int pulse_ns = (unsigned int)pulse_us * 1000;
    // duty_cycle is the ACTIVE duration: low for inversed, high for normal.
    // Convert the desired servo pulse to the driver's active duration.
    const unsigned int duty = (axis.inversed != axis.active_low)
                              ? period_ns - pulse_ns : pulse_ns;
    if (pwm_set_duty_cycle(axis.pwm, duty) != 0 ||
        (!axis.enabled && pwm_enable(axis.pwm, 1) != 0)) {
        LOG_ERROR("PTZ PWM%d output failed; angle not committed\n", (int)axis.pwm);
        return false;
    }
    Pwm_status actual = {};
    if (pwm_get_status(axis.pwm, &actual) != 0 || !actual.enabled ||
        std::abs((long long)actual.duty_cycle_ns - duty) > 1000 ||
        std::abs((long long)actual.period_ns - period_ns) > 1000) {
        LOG_ERROR("PTZ PWM%d readback mismatch; disabling output\n", (int)axis.pwm);
        pwm_enable(axis.pwm, 0);
        axis.enabled = false;
        return false;
    }
    axis.angle = angle;
    axis.enabled = true;
    LOG_DEBUG("PTZ PWM%d driver applied: command_deg=%d pulse_us=%d active=%s duty_ns=%u\n",
              (int)axis.pwm, angle, pulse_us, axis.active_low ? "low" : "high", duty);
    return true;
}

void Pantilt::move(int pan_step, int tilt_step) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Initialize both axes on the first command, then update only moving axes.
    if (pan_step || !pan_.enabled)
        apply(pan_, pan_.angle + pan_.direction * clamp(pan_step, -5, 5));
    if (tilt_step || !tilt_.enabled)
        apply(tilt_, tilt_.angle + tilt_.direction * clamp(tilt_step, -5, 5));
}
void Pantilt::onAjustPantilt(int pan_step, int tilt_step) {
    if (pan_step || tilt_step) move(pan_step, tilt_step);
}
void Pantilt::up(int step) { move(0, -clamp(step, 0, 5)); }
void Pantilt::down(int step) { move(0, clamp(step, 0, 5)); }
void Pantilt::left(int step) { move(-clamp(step, 0, 5), 0); }
void Pantilt::right(int step) { move(clamp(step, 0, 5), 0); }
void Pantilt::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    apply(pan_, pan_.home);
    apply(tilt_, tilt_.home);
}
void Pantilt::notify(string) {}
void Pantilt::update(Publisher*, string msg) {
    if (msg == "PTZ:reset") reset();
    else if (msg == "PTZ:up") up(3);
    else if (msg == "PTZ:down") down(3);
    else if (msg == "PTZ:left") left(3);
    else if (msg == "PTZ:right") right(3);
}
void Pantilt::shutdown(Axis& axis) {
    if (axis.ready) {
        pwm_enable(axis.pwm, 0);
        pwm_deinit(axis.pwm);
    }
}
Pantilt::~Pantilt() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown(pan_);
    shutdown(tilt_);
}
