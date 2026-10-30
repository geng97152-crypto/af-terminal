#include "pwm.h"
#include <errno.h>

#define SYSFS_PWM_PATH "/sys/class/pwm/pwmchip"
#define PWM10_M2_OVERLAY "/sys/kernel/config/device-tree/overlays/pwm10m2"

static int sysfs_write(const char *path, const char *value);

static int wait_for_path(const char *path, int timeout_ms) {
    int waited_ms = 0;
    while (access(path, F_OK) != 0 && waited_ms < timeout_ms) {
        usleep(20 * 1000);
        waited_ms += 20;
    }
    return access(path, F_OK) == 0 ? 0 : -1;
}

/*
 * The older image used on this board creates an incomplete PWM10_M1 overlay
 * and may also leave pwm10m2 loaded.  Prepare the two servo pins explicitly
 * at runtime so the application does not depend on reflashing the image.
 */
static int pwm_prepare_pinmux(int pwm_num) {
    char chip_path[64];
    int rc = 0;

    if (pwm_num == 10) {
        char status_path[160];
        snprintf(status_path, sizeof(status_path), "%s/status", PWM10_M2_OVERLAY);
        if (access(PWM10_M2_OVERLAY, F_OK) == 0) {
            if (access(status_path, F_OK) == 0)
                sysfs_write(status_path, "0");
            if (rmdir(PWM10_M2_OVERLAY) != 0 && errno != ENOENT)
                fprintf(stderr, "Failed to remove pwm10m2 overlay: %s\n", strerror(errno));
        }
        rc = system("iomux 1 22 3 >/dev/null 2>&1");
    } else if (pwm_num == 11) {
        rc = system("iomux 1 23 3 >/dev/null 2>&1");
    }

    if ((pwm_num == 10 || pwm_num == 11) && rc != 0) {
        fprintf(stderr, "Failed to configure PWM%d pinmux with iomux\n", pwm_num);
        return -1;
    }

    snprintf(chip_path, sizeof(chip_path), "%s%d", SYSFS_PWM_PATH, pwm_num);
    if (wait_for_path(chip_path, 1000) != 0) {
        fprintf(stderr, "PWM chip not found: %s\n", chip_path);
        return -1;
    }
    return 0;
}

/**
 * @brief 写入 sysfs 文件。
 * 
 * @param path 文件路径。
 * @param value 要写入的值。
 * @return 成功返回 0，失败返回 -1。
 */
static int sysfs_write(const char *path, const char *value) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }
    ssize_t expected = (ssize_t)strlen(value);
    ssize_t written = write(fd, value, (size_t)expected);
    close(fd);
    if (written != expected) {
        fprintf(stderr, "Failed to write %s: %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * @brief 读取 sysfs 文件。
 * 
 * @param path 文件路径。
 * @param value 读取的值。
 * @param size 读取的大小。
 * @return 成功返回 0，失败返回 -1。
 */
static int sysfs_read(const char *path, char *value, size_t size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }
    if (size == 0) {
        close(fd);
        return -1;
    }
    ssize_t count = read(fd, value, size - 1);
    close(fd);
    if (count < 0)
        return -1;
    value[count] = '\0';
    return 0;
}

/**
 * @brief 生成 PWM 文件路径.
 *
 * 此函数根据给定的 PWM 通道号生成 PWM 操作文件的路径.
 *
 * @param path 用于存储生成的路径的缓冲区.
 * @param pwm_num PWM 通道号.
 * @param filename 需要拼接的文件名.
 */
void pwm_generate_path(char *path, int pwm_num, const char *filename) {
    sprintf(path, "%s%d/pwm0/%s", SYSFS_PWM_PATH, pwm_num, filename);
}

/**
 * @brief 初始化 PWM 通道.
 *
 * 此函数导出指定的 PWM 通道，并为后续操作做好准备.
 *
 * @param pwm_num PWM 通道号.
 * @return 成功返回 0，失败返回 -1.
 */
int pwm_init(enum PWM_num pwm_num) {
    char path[64];

    if (pwm_prepare_pinmux(pwm_num) != 0)
        return -1;
    
    // Check if PWM is already exported
    sprintf(path, "%s%d/pwm0", SYSFS_PWM_PATH, pwm_num);
    if (access(path, F_OK) == 0) {
        fprintf(stdout, "PWM%d already exported\n", pwm_num);
        return 0;
    }
    
    // Export the PWM
    sprintf(path, "%s%d/export", SYSFS_PWM_PATH, pwm_num);
    if (sysfs_write(path, "0") != 0) {
        sprintf(path, "%s%d/pwm0", SYSFS_PWM_PATH, pwm_num);
        if (access(path, F_OK) != 0)
            return -1;
    }

    sprintf(path, "%s%d/pwm0", SYSFS_PWM_PATH, pwm_num);
    if (wait_for_path(path, 1000) != 0) {
        fprintf(stderr, "PWM%d export timed out\n", pwm_num);
        return -1;
    }
    return 0;
}

/**
 * @brief 释放 PWM 通道.
 *
 * 此函数取消导出 PWM 通道，释放资源.
 *
 * @param pwm_num PWM 通道号.
 * @return 成功返回 0，失败返回 -1.
 */
int pwm_deinit(enum PWM_num pwm_num) {
    char path[64];
    
    // Unexport the PWM
    sprintf(path, "%s%d/unexport", SYSFS_PWM_PATH, pwm_num);
    return sysfs_write(path, "0");
}

/**
 * @brief 设置 PWM 周期.
 *
 * 设置指定 PWM 通道的周期（单位：纳秒）.
 *
 * @param pwm_num PWM 通道号.
 * @param period_ns PWM 周期（纳秒）.
 * @return 成功返回 0，失败返回 -1.
 */
int pwm_set_period(enum PWM_num pwm_num, unsigned int period_ns) {
    char path[64];
    char value[32];
    
    // Write period to sysfs
    pwm_generate_path(path, pwm_num, "period");
    sprintf(value, "%u", period_ns);
    return sysfs_write(path, value);
}

/**
 * @brief 设置 PWM 占空比.
 *
 * 设置指定 PWM 通道的占空比（单位：纳秒）.
 *
 * @param pwm_num PWM 通道号.
 * @param duty_cycle_ns PWM 占空比（纳秒）.
 * @return 成功返回 0，失败返回 -1.
 */
int pwm_set_duty_cycle(enum PWM_num pwm_num, unsigned int duty_cycle_ns) {
    char path[64];
    char value[32];
    
    // Write duty cycle to sysfs
    pwm_generate_path(path, pwm_num, "duty_cycle");
    sprintf(value, "%u", duty_cycle_ns);
    return sysfs_write(path, value);
}

/**
 * @brief 设置 PWM 输出极性.
 *
 * 设置指定 PWM 通道的输出极性.
 *
 * @param pwm_num PWM 通道号.
 * @param polarity 输出极性（normal/inversed）.
 * @return 成功返回 0，失败返回 -1.
 */
int pwm_set_polarity(enum PWM_num pwm_num, const char* polarity) {
    char path[64];
    
    // Write polarity to sysfs
    pwm_generate_path(path, pwm_num, "polarity");
    return sysfs_write(path, polarity);
}

/**
 * @brief 启用/禁用 PWM 通道.
 *
 * @param pwm_num PWM 通道号.
 * @param enable 1 = 启用，0 = 禁用.
 * @return 成功返回 0，失败返回 -1.
 */
int pwm_enable(enum PWM_num pwm_num, int enable) {
    char path[64];
    char value[8];
    
    // Write enable/disable to sysfs
    pwm_generate_path(path, pwm_num, "enable");
    sprintf(value, "%d", enable);
    return sysfs_write(path, value);
}

/**
 * @brief 获取 PWM 通道的状态.
 *
 * 读取 PWM 通道的当前周期、占空比和启用状态.
 *
 * @param pwm_num PWM 通道号.
 * @param status 保存 PWM 状态.
 * @return 成功返回 0，失败返回 -1.
 */
int pwm_get_status(enum PWM_num pwm_num, struct Pwm_status *status) {
    char path[64];
    char value[32];

    // Read period
    pwm_generate_path(path, pwm_num, "period");
    if (sysfs_read(path, value, sizeof(value)) == 0) {
        status->period_ns = atoi(value);
    } else {
        return -1;
    }

    // Read duty cycle
    pwm_generate_path(path, pwm_num, "duty_cycle");
    if (sysfs_read(path, value, sizeof(value)) == 0) {
        status->duty_cycle_ns = atoi(value);
    } else {
        return -1;
    }

    // Read enabled status
    pwm_generate_path(path, pwm_num, "enable");
    if (sysfs_read(path, value, sizeof(value)) == 0) {
        status->enabled = atoi(value);
    } else {
        return -1;
    }

    return 0;
}
