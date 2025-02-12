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

#pragma once

#define LOG_TAG "FingerprintHal"

#include <aidl/android/hardware/biometrics/common/SensorStrength.h>
#include <aidl/android/hardware/biometrics/fingerprint/ISessionCallback.h>
#include <android/binder_to_string.h>
#include <string>

#include <random>

#include <aidl/android/hardware/biometrics/fingerprint/SensorLocation.h>
#include <future>
#include <vector>

#include "LockoutTracker.h"

#include <fstream>
#include "fingerprint-xiaomi.h"

using namespace ::aidl::android::hardware::biometrics::common;

namespace aidl::android::hardware::biometrics::fingerprint {

class FingerprintEngine {
  public:
    FingerprintEngine();
    virtual ~FingerprintEngine() {}

    void setActiveGroup(int userId);
    void generateChallengeImpl(ISessionCallback* cb);
    void revokeChallengeImpl(ISessionCallback* cb, int64_t challenge);
    virtual void enrollImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat,
                            const std::future<void>& cancel);
    virtual void authenticateImpl(ISessionCallback* cb, int64_t operationId,
                                  const std::future<void>& cancel);
    virtual void detectInteractionImpl(ISessionCallback* cb, const std::future<void>& cancel);
    void enumerateEnrollmentsImpl(ISessionCallback* cb);
    void removeEnrollmentsImpl(ISessionCallback* cb, const std::vector<int32_t>& enrollmentIds);
    void getAuthenticatorIdImpl(ISessionCallback* cb);
    void invalidateAuthenticatorIdImpl(ISessionCallback* cb);
    void resetLockoutImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat);

    virtual ndk::ScopedAStatus onPointerDownImpl(int32_t pointerId, int32_t x, int32_t y,
                                                 float minor, float major);

    virtual ndk::ScopedAStatus onPointerUpImpl(int32_t pointerId);

    virtual ndk::ScopedAStatus onUiReadyImpl();

    virtual SensorLocation getSensorLocation();
  
  protected:
    ISessionCallback* mCb;

  private:
    static constexpr int32_t FINGERPRINT_ACQUIRED_VENDOR_BASE = 1000;
    static constexpr int32_t FINGERPRINT_ERROR_VENDOR_BASE = 1000;
    void clearLockout(ISessionCallback* cb, bool dueToTimeout = false);
    void waitForFingerDown(ISessionCallback* cb, const std::future<void>& cancel);

    fingerprint_device_t* openFingerprintHal(const char* class_name,
                                                        const char* module_id);

    template <typename T>
    void set(const std::string &path, const T &value);
    void setFodStatus(int value);

    fingerprint_device_t* mDevice;
    void setFingerStatus(bool pressed);

  protected:
    // lockout timer
    void lockoutTimerExpired(ISessionCallback* cb);
    bool isLockoutTimerSupported;
    bool isLockoutTimerStarted;
    bool isLockoutTimerAborted;

  public:
    void startLockoutTimer(int64_t timeout, ISessionCallback* cb);
    bool getLockoutTimerStarted() { return isLockoutTimerStarted; };

    LockoutTracker mLockoutTracker;
    void onAcquired(int32_t result, int32_t vendorCode);
    std::pair<AcquiredInfo, int32_t> convertAcquiredInfo(int32_t code);
    std::pair<Error, int32_t> convertError(int32_t code);
    bool checkSensorLockout(ISessionCallback*);
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
