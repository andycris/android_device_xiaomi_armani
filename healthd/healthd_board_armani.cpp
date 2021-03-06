/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/klog.h>
#include <cutils/android_reboot.h>
#include <healthd.h>
#include "minui/minui.h"

#define ARRAY_SIZE(x)           (sizeof(x)/sizeof(x[0]))

#define BACKLIGHT_PATH          "/sys/devices/fd900000.qcom,mdss_mdp/qcom,mdss_fb_primary.154/leds/lcd-backlight/brightness"

#define CHARGING_ENABLED_PATH   "/sys/devices/qpnp-charger-eb5ffe00/power_supply/battery/charging_enabled"

#define LOGE(x...) do { KLOG_ERROR("charger", x); } while (0)
#define LOGW(x...) do { KLOG_WARNING("charger", x); } while (0)
#define LOGV(x...) do { KLOG_DEBUG("charger", x); } while (0)

#define STR_LEN 8
void healthd_board_mode_charger_draw_battery(
                struct android::BatteryProperties *batt_prop)
{
    char cap_str[STR_LEN];
    int x, y;
    int str_len_px;
    static int char_height = -1, char_width = -1;

    if (char_height == -1 && char_width == -1)
        gr_font_size(&char_width, &char_height);
    snprintf(cap_str, (STR_LEN - 1), "%d%%", batt_prop->batteryLevel);
    str_len_px = gr_measure(cap_str);
    x = (gr_fb_width() - str_len_px) / 2;
    y = (gr_fb_height() + char_height) / 2;
    gr_color(0xa4, 0xc6, 0x39, 255);
    gr_text(x, y, cap_str, 0);
}

void healthd_board_mode_charger_battery_update(
                struct android::BatteryProperties*)
{

}

#define BACKLIGHT_ON_LEVEL    100
#define BACKLIGHT_OFF_LEVEL    0
void healthd_board_mode_charger_set_backlight(bool en)
{
    int fd;
    char buffer[10];

    if (access(BACKLIGHT_PATH, R_OK | W_OK) != 0)
    {
        LOGW("Backlight control not support\n");
        return;
    }

    memset(buffer, '\0', sizeof(buffer));
    fd = open(BACKLIGHT_PATH, O_RDWR);
    if (fd < 0) {
        LOGE("Could not open backlight node : %s\n", strerror(errno));
        goto cleanup;
    }
    LOGV("set backlight status to %d\n", en);
    if (en)
        snprintf(buffer, sizeof(buffer), "%d\n", BACKLIGHT_ON_LEVEL);
    else
        snprintf(buffer, sizeof(buffer), "%d\n", BACKLIGHT_OFF_LEVEL);

    if (write(fd, buffer,strlen(buffer)) < 0) {
        LOGE("Could not write to backlight node : %s\n", strerror(errno));
        goto cleanup;
    }
cleanup:
    if (fd >= 0)
        close(fd);
}

void healthd_board_mode_charger_init()
{
    int ret;
    char buff[8] = "\0";
    int charging_enabled = 0;
    int fd;

    /* check the charging is enabled or not */
    fd = open(CHARGING_ENABLED_PATH, O_RDONLY);
    if (fd < 0)
        return;
    ret = read(fd, buff, sizeof(buff));
    close(fd);
    if (ret > 0 && sscanf(buff, "%d\n", &charging_enabled)) {
        /* if charging is disabled, reboot and exit power off charging */
        if (charging_enabled)
            return;
        LOGW("android charging is disabled, exit!\n");
        android_reboot(ANDROID_RB_RESTART, 0, 0);
    }
}

void healthd_board_init(struct healthd_config*)
{
    // use defaults
}

int healthd_board_battery_update(struct android::BatteryProperties*)
{
    // return 0 to log periodic polled battery status to kernel log
    return 1;
}