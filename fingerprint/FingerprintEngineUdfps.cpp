/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "FingerprintEngineUdfps.h"

#include <android-base/logging.h>

#include <fingerprint.sysprop.h>

#include "Fingerprint.h"
#include "util/CancellationSignal.h"
#include "util/Util.h"

#undef LOG_TAG
#define LOG_TAG "FingerprintHalUdfps"

using namespace ::android::fingerprint::shennong;

namespace aidl::android::hardware::biometrics::fingerprint {

FingerprintEngineUdfps::FingerprintEngineUdfps()
    : FingerprintEngine(), mPointerDownTime(0), mUiReadyTime(0) {}

SensorLocation FingerprintEngineUdfps::defaultSensorLocation() {
    return SensorLocation{.sensorLocationX = defaultSensorLocationX,
                          .sensorLocationY = defaultSensorLocationY,
                          .sensorRadius = defaultSensorRadius};
}

ndk::ScopedAStatus FingerprintEngineUdfps::onPointerDownImpl(int32_t /*pointerId*/,
                                                                 int32_t /*x*/, int32_t /*y*/,
                                                                 float /*minor*/, float /*major*/) {
    BEGIN_OP(0);
    // verify whetehr touch coordinates/area matching sensor location ?
    mPointerDownTime = Util::getSystemNanoTime();
    if (Fingerprint::cfg().get<bool>("control_illumination")) {
        fingerDownAction();
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FingerprintEngineUdfps::onPointerUpImpl(int32_t /*pointerId*/) {
    BEGIN_OP(0);
    mUiReadyTime = 0;
    mPointerDownTime = 0;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FingerprintEngineUdfps::onUiReadyImpl() {
    BEGIN_OP(0);

    if (Util::hasElapsed(mPointerDownTime, uiReadyTimeoutInMs * 100)) {
        LOG(ERROR) << "onUiReady() arrives too late after onPointerDown()";
    } else {
        fingerDownAction();
    }
    return ndk::ScopedAStatus::ok();
}

void FingerprintEngineUdfps::fingerDownAction() {
    FingerprintEngine::fingerDownAction();
    mUiReadyTime = 0;
    mPointerDownTime = 0;
}

void FingerprintEngineUdfps::updateContext(WorkMode mode, ISessionCallback* cb,
                                               std::future<void>& cancel, int64_t operationId,
                                               const keymaster::HardwareAuthToken& hat) {
    FingerprintEngine::updateContext(mode, cb, cancel, operationId, hat);
    mPointerDownTime = 0;
    mUiReadyTime = 0;
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
