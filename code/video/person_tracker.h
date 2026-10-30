#pragma once
#include "lcd_detection.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Coordinates are normalized to the full camera frame, before LCD cropping.
struct PersonTrackerConfig {
    bool enabled = true;
    float min_confidence = 0.25f;
    float view_width = 0.5f;
    float dead_zone_x = 0.08f;
    float dead_zone_y = 0.08f;
    int interval_ms = 200;
    int lost_timeout_ms = 800;
    int max_step = 3;
};

struct PersonTrackCommand {
    bool send = false;
    int pan = 0;
    int tilt = 0;
    float error_x = 0;
    float error_y = 0;
};

class PersonTracker {
public:
    explicit PersonTracker(PersonTrackerConfig config) : config_(config) {}

    PersonTrackCommand update(const std::vector<LcdDetection>& detections, long long now) {
        PersonTrackCommand command;
        if (!config_.enabled) return command;
        if (locked_ && now - last_seen_ > config_.lost_timeout_ms) clear();

        const LcdDetection* selected = nullptr;
        float best = -1e9f;
        for (const auto& box : detections) {
            if (box.label != "person" || !std::isfinite(box.confidence) ||
                box.confidence < config_.min_confidence || !valid(box)) continue;
            float score;
            if (locked_) {
                float dx = centerX(box) - centerX(target_);
                float dy = centerY(box) - centerY(target_);
                float distance = std::sqrt(dx * dx + dy * dy);
                float overlap = iou(box, target_);
                if (overlap < 0.05f && distance > 0.30f) continue;
                score = 3 * overlap - distance + 0.1f * box.confidence;
            } else {
                float area = (box.right - box.left) * (box.bottom - box.top);
                score = area + 0.3f * box.confidence;
            }
            if (score > best) { best = score; selected = &box; }
        }
        if (!selected) {
            // Keep the last confirmed identity briefly, but never command
            // a servo without a detection in the current frame.
            return command;
        }
        if (!locked_) {
            target_ = *selected;
            locked_ = true;
        } else {
            target_.left = (target_.left + selected->left) * 0.5f;
            target_.right = (target_.right + selected->right) * 0.5f;
            target_.top = (target_.top + selected->top) * 0.5f;
            target_.bottom = (target_.bottom + selected->bottom) * 0.5f;
        }
        last_seen_ = now;
        const float left = (1 - config_.view_width) * 0.5f;
        command.error_x = axisError(target_.left, target_.right, left,
                                    left + config_.view_width, config_.dead_zone_x);
        command.error_y = axisError(target_.top, target_.bottom, 0, 1,
                                    config_.dead_zone_y);
        if (sent_ && now - last_command_ < config_.interval_ms)
            return command;
        command.pan = step(command.error_x);
        command.tilt = step(command.error_y);
        command.send = command.pan != 0 || command.tilt != 0;
        if (command.send) { last_command_ = now; sent_ = true; }
        return command;
    }

    void clear() { locked_ = false; sent_ = false; }

private:
    static bool valid(const LcdDetection& b) {
        return std::isfinite(b.left) && std::isfinite(b.top) &&
               std::isfinite(b.right) && std::isfinite(b.bottom) &&
               b.left >= 0 && b.top >= 0 && b.right <= 1 && b.bottom <= 1 &&
               b.right > b.left && b.bottom > b.top;
    }
    static float centerX(const LcdDetection& b) { return (b.left + b.right) * 0.5f; }
    static float centerY(const LcdDetection& b) { return (b.top + b.bottom) * 0.5f; }
    static float iou(const LcdDetection& a, const LcdDetection& b) {
        float w = std::max(0.f, std::min(a.right,b.right)-std::max(a.left,b.left));
        float h = std::max(0.f, std::min(a.bottom,b.bottom)-std::max(a.top,b.top));
        float intersection = w * h;
        float total = (a.right-a.left)*(a.bottom-a.top) +
                      (b.right-b.left)*(b.bottom-b.top)-intersection;
        return total > 0 ? intersection / total : 0;
    }
    static float axisError(float start, float end, float low, float high, float dead) {
        float span = high - low;
        float error = ((start + end) * 0.5f - (low + high) * 0.5f) / span;
        // A box larger than the visible area cannot be fitted by rotation alone.
        if (end - start < span * 0.9f) {
            if (start < low + span * 0.05f)
                error = std::min(error, (start - low - span * 0.05f) / span);
            if (end > high - span * 0.05f)
                error = std::max(error, (end - high + span * 0.05f) / span);
        }
        return std::fabs(error) <= dead ? 0 : error;
    }
    int step(float error) const {
        if (error == 0) return 0;
        int magnitude = std::max(1, std::min(config_.max_step,
                              (int)std::ceil(std::fabs(error) * 8)));
        return error > 0 ? magnitude : -magnitude;
    }
    PersonTrackerConfig config_;
    LcdDetection target_;
    bool locked_ = false;
    bool sent_ = false;
    long long last_seen_ = 0;
    long long last_command_ = 0;
};
