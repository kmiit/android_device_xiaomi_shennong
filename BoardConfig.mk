#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/xiaomi/shennong

# Display
TARGET_SCREEN_DENSITY := 560

# OTA assert
TARGET_OTA_ASSERT_DEVICE := shennong

# Partitions
BOARD_SUPER_PARTITION_SIZE := 8321499136

# Include the common OEM chipset BoardConfig.
include device/xiaomi/sm8650-common/BoardConfigCommon.mk

BOARD_VENDOR_KERNEL_MODULES_LOAD += \
	cs35l43_dlkm.ko \
	goodix_fod.ko \
	gpio-jingshang.ko \
	synaptics_tcm2.ko
