/*
 * Copyright (C) 2021 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aidl/android/hardware/power/BnPower.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <sys/ioctl.h>

#define COMMON_DATA_CMD 0
#define SELECT_TOUCH_ID 3

#define SET_CUR_VALUE 0
#define TOUCH_DOUBLETAP_MODE 14
#define TOUCH_MAGIC 'T'
#define TOUCH_DEV_PATH "/dev/xiaomi-touch"
#define TOUCH_ID 0
#define CMD_DATA_BUF_SIZE 128

namespace {
typedef struct {
	int8_t touch_id = TOUCH_ID;
	uint8_t cmd = SET_CUR_VALUE;
	uint16_t mode;
	uint16_t data_len = CMD_DATA_BUF_SIZE;
	int32_t data_buf[CMD_DATA_BUF_SIZE];
} common_data_t;
}

#define XIAOMI_IOC_COMMON_DATA _IOW(TOUCH_MAGIC, COMMON_DATA_CMD, common_data_t)
#define XIAOMI_IOC_SELECT_TOUCH_ID _IOW(TOUCH_MAGIC, SELECT_TOUCH_ID, unsigned long)

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace impl {

using ::aidl::android::hardware::power::Mode;

bool isDeviceSpecificModeSupported(Mode type, bool* _aidl_return) {
    switch (type) {
        case Mode::DOUBLE_TAP_TO_WAKE:
            *_aidl_return = true;
            return true;
        default:
            return false;
    }
}

bool setDeviceSpecificMode(Mode type, bool enabled) {
    switch (type) {
        case Mode::DOUBLE_TAP_TO_WAKE: {
            int fd = open(TOUCH_DEV_PATH, O_RDWR);
            common_data_t data = {
                .mode = TOUCH_DOUBLETAP_MODE,
                .data_buf = {enabled ? 1 : 0},
            };
            // Select touch id in this fd first
            ioctl(fd, XIAOMI_IOC_SELECT_TOUCH_ID, TOUCH_ID);
            ioctl(fd, XIAOMI_IOC_COMMON_DATA, &data);
            close(fd);
            return true;
        }
        default:
            return false;
    }
}

}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl
