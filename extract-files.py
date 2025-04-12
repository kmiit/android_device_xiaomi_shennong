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
from extract_utils.fixups_lib import (
    lib_fixup_remove,
    lib_fixups,
    lib_fixups_user_type,
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

def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}-{partition}' if partition == 'vendor' else None

lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'vendor.qti.diaghal@1.0',
        'vendor.qti.imsrtpservice@3.0',
        'vendor.qti.imsrtpservice@3.1',
        'vendor.qti.ImsRtpService-V1-ndk'
    ): lib_fixup_vendor_suffix,
    (
        'android.hardware.graphics.composer3-V2-ndk',
        'audio.primary.pineapple',
        'libagmclient',
        'libagmmixer',
        'libpalclient',
    ): lib_fixup_remove,
}

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
        .add_needed('libcodec2_shim.so'),
    'vendor/lib64/vendor.libdpmframework.so': blob_fixup()
        .add_needed('libhidlbase_shim.so'),
    (
        'odm/lib64/camera/com.qti.actuator.shennong_ofilm_s5kjn1_dw9800v_ultra_actuator.so',
        'odm/lib64/camera/com.qti.actuator.shennong_ofilm_s5kjn1_gt9764v_tele_actuator.so',
        'odm/lib64/camera/com.qti.actuator.shennong_semco_ovx9000_ak7316_wide_ii_actuator.so',
        'odm/lib64/camera/com.qti.actuator.shennong_sunny_ovx9000_ak7316_wide_actuator.so',
        'odm/lib64/camera/com.qti.aperture.shennong_semco_ovx9000_ak7316_wide_ii_aperture.so',
        'odm/lib64/camera/com.qti.aperture.shennong_sunny_ovx9000_ak7316_wide_aperture.so',
        'odm/lib64/camera/com.qti.eeprom.shennong_ofilm_s5kjn1_gt24p128f_tele_eeprom.so',
        'odm/lib64/camera/com.qti.eeprom.shennong_ofilm_s5kjn1_gt24p128f_ultra_eeprom.so',
        'odm/lib64/camera/com.qti.eeprom.shennong_semco_ovx9000_p24c256f_wide_ii_eeprom.so',
        'odm/lib64/camera/com.qti.eeprom.shennong_sunny_ov32b40_p24c64f_front_eeprom.so',
        'odm/lib64/camera/com.qti.eeprom.shennong_sunny_ovx9000_p24c256f_wide_eeprom.so',
        'odm/lib64/camera/com.qti.ois.shennong_tele_bu24618_ois.so',
        'odm/lib64/camera/com.qti.ois.shennong_wide_bu24721_ois_ii.so',
        'odm/lib64/camera/com.qti.ois.shennong_wide_s10_ois.so',
        'odm/lib64/camera/com.qti.sensor.shennong_ofilm_s5kjn1_tele.so',
        'odm/lib64/camera/com.qti.sensor.shennong_ofilm_s5kjn1_ultra.so',
        'odm/lib64/camera/com.qti.sensor.shennong_semco_ovx9000_wide_ii.so',
        'odm/lib64/camera/com.qti.sensor.shennong_sunny_ov32b40_front.so',
        'odm/lib64/camera/com.qti.sensor.shennong_sunny_ovx9000_wide.so',
        'odm/lib64/camera/components/com.jigan.node.videobokeh.so',
        'odm/lib64/camera/components/com.mi.node.aiasd.so',
        'odm/lib64/camera/components/com.mi.node.dlengine.so',
        'odm/lib64/camera/components/com.mi.node.mawsaliency.so',
        'odm/lib64/camera/components/com.mi.node.rearvideo.so',
        'odm/lib64/camera/components/com.mi.node.skinbeautifier.so',
        'odm/lib64/camera/components/com.mi.node.videobokeh.so',
        'odm/lib64/camera/components/com.mi.node.videofilter.so',
        'odm/lib64/camera/components/com.mi.node.videonight.so',
        'odm/lib64/camera/components/com.qti.node.aon.so',
        'odm/lib64/camera/components/com.qti.node.depth.so',
        'odm/lib64/camera/components/com.qti.node.depthprovider.so',
        'odm/lib64/camera/components/com.qti.node.dewarp.so',
        'odm/lib64/camera/components/com.qti.node.eisv2.so',
        'odm/lib64/camera/components/com.qti.node.eisv3.so',
        'odm/lib64/camera/components/com.qti.node.evadepth.so',
        'odm/lib64/camera/components/com.qti.node.gme.so',
        'odm/lib64/camera/components/com.qti.node.gyrornn.so',
        'odm/lib64/camera/components/com.qti.node.hdr10pgen.so',
        'odm/lib64/camera/components/com.qti.node.hdr10phist.so',
        'odm/lib64/camera/components/com.qti.node.itofpreprocess.so',
        'odm/lib64/camera/components/com.qti.node.ml.so',
        'odm/lib64/camera/components/com.qti.node.mlinference.so',
        'odm/lib64/camera/components/com.qti.node.pixelstats.so',
        'odm/lib64/camera/components/com.qti.node.seg.so',
        'odm/lib64/camera/components/com.qti.node.swec.so',
        'odm/lib64/camera/components/com.qti.node.swregistration.so',
        'odm/lib64/camera/components/com.qti.node.swvrt.so',
        'odm/lib64/camera/components/com.qti.stats.cnndriver.so',
        'odm/lib64/camera/components/com.xiaomi.node.smooth_transition.so',
        'odm/lib64/camera/components/libcamxevainterface.so',
        'odm/lib64/camera/components/libdepthmapwrapper_itof.so',
        'odm/lib64/camera/components/libdepthmapwrapper_secure.so',
        'odm/lib64/camera/libchxlogicalcameratable.so',
        'odm/lib64/com.qti.camx.chiiqutils.so',
        'odm/lib64/com.qti.chiusecaseselector.so',
        'odm/lib64/com.qti.feature2.afbrckt.so',
        'odm/lib64/com.qti.feature2.anchorsync.so',
        'odm/lib64/com.qti.feature2.demux.so',
        'odm/lib64/com.qti.feature2.derivedoffline.so',
        'odm/lib64/com.qti.feature2.fusion.so',
        'odm/lib64/com.qti.feature2.generic.so',
        'odm/lib64/com.qti.feature2.gs.sm8650.so',
        'odm/lib64/hw/camera.qcom.sm8650.so',
        'odm/lib64/hw/com.qti.chi.offline.so',
        'odm/lib64/hw/com.qti.chi.override.so',
        'odm/lib64/com.qti.feature2.hdr.so',
        'odm/lib64/com.qti.feature2.mcreprocrt.so',
        'odm/lib64/com.qti.feature2.memcpy.so',
        'odm/lib64/com.qti.feature2.metadataserializer.so',
        'odm/lib64/com.qti.feature2.mfsr.so',
        'odm/lib64/com.qti.feature2.ml.so',
        'odm/lib64/com.qti.feature2.mux.so',
        'odm/lib64/com.qti.feature2.offlinestatsregeneration.so',
        'odm/lib64/com.qti.feature2.qcfa.so',
        'odm/lib64/com.qti.feature2.rawhdr.so',
        'odm/lib64/com.qti.feature2.realtimeserializer.so',
        'odm/lib64/com.qti.feature2.rt.so',
        'odm/lib64/com.qti.feature2.rtmcx.so',
        'odm/lib64/com.qti.feature2.serializer.so',
        'odm/lib64/com.qti.feature2.statsregeneration.so',
        'odm/lib64/com.qti.feature2.stub.so',
        'odm/lib64/com.qti.feature2.swmf.so',
        'odm/lib64/com.qti.qseeutils.so',
        'odm/lib64/com.qualcomm.mcx.distortionmapper.so',
        'odm/lib64/com.qualcomm.mcx.linearmapper.so',
        'odm/lib64/com.qualcomm.mcx.nonlinearmapper.so',
        'odm/lib64/com.qualcomm.mcx.policy.mfl.so',
        'odm/lib64/com.qualcomm.qti.mcx.usecase.extension.so',
        'odm/lib64/libcamerapostproc.so',
        'odm/lib64/libcamxifestriping.so',
        'odm/lib64/libfastmessage.so',
        'odm/lib64/libhme.so',
        'odm/lib64/libipebpsstriping.so',
        'odm/lib64/libipebpsstriping170.so',
        'odm/lib64/libmctfengine_stub.so',
        'odm/lib64/libmfec.so',
        'odm/lib64/libmmcamera_bestats.so',
        'odm/lib64/libmmcamera_cac.so',
        'odm/lib64/libmmcamera_lscv35.so',
        'odm/lib64/libmmcamera_mfnr.so',
        'odm/lib64/libmmcamera_mfnr_t4.so',
        'odm/lib64/libofflinefeatureintf.so',
        'odm/lib64/libtunningmemhook.so',
        'odm/lib64/libubifocus.so',
        'odm/lib64/vendor.qti.hardware.camera.aon-service-impl.so',
        'odm/lib64/vendor.qti.hardware.camera.offlinecamera-service-impl.so',
        'odm/lib64/libcamxhwnodecontext.so',
        'odm/lib64/libcamximageformatutils.so',
        'odm/lib64/libcamxncsdatafactory.so',
        'odm/lib64/libchifeature2.so',
        'odm/lib64/libcom.xiaomi.mawutilsold.so',
        'odm/lib64/libcommonchiutils.so',
        'odm/lib64/libipebpsstriping480.so',
        'odm/lib64/libisphwsetting.so',
        'odm/lib64/libjpege.so',
        'odm/lib64/libmmcamera_pdpc.so',
        'odm/lib64/libopestriping.so',
        'odm/lib64/libtfestriping.so',
        'odm/lib64/vendor.qti.hardware.camera.postproc@1.0-service-impl.so'
    ): blob_fixup()
        .replace_needed('android.hardware.graphics.allocator-V1-ndk.so', 'android.hardware.graphics.allocator-V2-ndk.so'),
    (
        'odm/bin/hw/vendor.xiaomi.sensor.citsensorservice.aidl',
        # 'vendor/bin/hw/vendor.qti.hardware.display.composer-service',
        'vendor/lib64/libdisplaydebug.so'
    ): blob_fixup()
        .replace_needed('android.hardware.graphics.composer3-V2-ndk.so', 'android.hardware.graphics.composer3-V3-ndk.so'),
    (
        'odm/lib64/libcamxcommonutils.so',
        'vendor/lib64/libcameraopt.so',
    ): blob_fixup()
        .add_needed('libprocessgroup_shim.so'),
    'odm/lib64/hw/camera.qcom.so': blob_fixup()
        .add_needed('libprocessgroup_shim.so')
        .replace_needed('android.hardware.graphics.allocator-V1-ndk.so', 'android.hardware.graphics.allocator-V2-ndk.so'),
    'vendor/bin/pnscr-sst': blob_fixup()
        .add_needed('libbase_shim.so'),
    'vendor/etc/seccomp_policy/gnss@2.0-qsap-location.policy': blob_fixup()
        .add_line_if_missing('sched_get_priority_min: 1')
        .add_line_if_missing('sched_get_priority_max: 1'),
}  # fmt: skip

module = ExtractUtilsModule(
    DEVICE,
    VENDOR,
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
    add_firmware_proprietary_file=True,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
