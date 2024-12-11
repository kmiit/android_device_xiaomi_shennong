#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

DEVICE = "shennong"
VENDOR = "xiaomi"

from extract_utils.file import File
from extract_utils.fixups_blob import (
    BlobFixupCtx,
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    "device/xiaomi/shennong",
    "hardware/qcom-caf/sm8650",
    "hardware/qcom-caf/wlan",
    "hardware/xiaomi",
    "vendor/qcom/opensource/commonsys-intf/display",
    "vendor/qcom/opensource/dataservices",
]

blob_fixups: blob_fixups_user_type = {
    (
        'odm/etc/camera/enhance_motiontuning.xml',
        'odm/etc/camera/night_motiontuning.xml',
        'odm/etc/camera/motiontuning.xml'
    ): blob_fixup()
        .regex_replace('xml=version', 'xml version'),
    'vendor/etc/sensors/hals.conf': blob_fixup()
        .regex_replace('sensors.ultrasoundproximity.so', '')
        .regex_replace('vl53l8.hal@2.0.so', ''),
    'vendor/lib64/libqcodec2_core.so': blob_fixup()
        .add_needed('qcodec2_shim.so'),
    'vendor/lib64/vendor.libdpmframework.so': blob_fixup()
        .add_needed('dpm_shim.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    DEVICE,
    VENDOR,
    blob_fixups=blob_fixups,
    namespace_imports=namespace_imports,
    add_firmware_proprietary_file=True,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
