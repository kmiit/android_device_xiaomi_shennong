#
# Copyright (C) 2023 The Android Open Source Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Inherit from shennong device.
$(call inherit-product, device/xiaomi/shennong/device.mk)

## Device identifier
PRODUCT_BRAND := Xiaomi
PRODUCT_DEVICE := shennong
PRODUCT_MANUFACTURER := Xiaomi
PRODUCT_NAME := lineage_shennong

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRIVATE_BUILD_DESC="shennong-user 15 AQ3A.240627.003 OS2.0.6.0.VNBCNXM release-keys"

BUILD_FINGERPRINT := Xiaomi/shennong/shennong:15/AQ3A.240627.003/OS2.0.6.0.VNBCNXM:user/release-keys

# GMS
PRODUCT_GMS_CLIENTID_BASE := android-xiaomi
