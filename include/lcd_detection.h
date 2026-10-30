#ifndef LCD_DETECTION_H
#define LCD_DETECTION_H

#include <string>

// Normalized coordinates in the camera image: [0.0, 1.0].
// Using normalized coordinates lets the 640x640 inference path and the
// 720x480 LCD path share the same detection result.
struct LcdDetection {
    float left;
    float top;
    float right;
    float bottom;
    float confidence;
    std::string label;
};

#endif
