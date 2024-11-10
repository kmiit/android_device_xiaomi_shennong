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
    "hardware/qcom-caf/sm8650",
    "hardware/qcom-caf/wlan",
    "hardware/xiaomi",
    "vendor/qcom/opensource/commonsys/display",
    "vendor/qcom/opensource/commonsys-intf/display",
    "vendor/qcom/opensource/dataservices",
    "vendor/qcom/opensource/display",
]

def fix_gettid(
    ctx: BlobFixupCtx,
    file: File,
    file_path: str,
    *args,
    **kargs,):
    with open(file_path, 'a') as f:
        f.writelines('\ngettid: 1')

blob_fixups: blob_fixups_user_type = {
    '*.xml': blob_fixup()
        .fix_xml(),
    ('vendor/etc/seccomp_policy/qsap_sensors.policy',
     'vendor/etc/seccomp_policy/atfwd@2.0.policy',
     'vendor/etc/seccomp_policy/gnss@2.0-qsap-location.policy',
     'vendor/etc/seccomp_policy/wfdhdcphalservice.policy'): blob_fixup()
        .call(fix_gettid),
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
