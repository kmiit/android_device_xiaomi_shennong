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

#include "FingerprintEngine.h"
#include <regex>
#include "Fingerprint.h"

#include <android-base/logging.h>
#include <android-base/parseint.h>

#include <fingerprint.sysprop.h>

#include "util/CancellationSignal.h"
#include "util/Util.h"

using namespace ::android::fingerprint::shennong;
using ::android::base::ParseInt;

namespace aidl::android::hardware::biometrics::fingerprint {

FingerprintEngine::FingerprintEngine()
    : isLockoutTimerSupported(true) {
    if (mDevice) {
        LOG(INFO) << "Fingerprint HAL already opened";
    } else {
        mDevice = openFingerprintHal();
        
        if (!mDevice) {
            LOG(ERROR) << "Can't open fingerprint HAL module, please check ro.hardware.${class}";
        } else {
            LOG(INFO) << "Opened fingerprint HAL module";
        }
    }
}

void FingerprintEngine::setActiveGroup(int userId) {
    LOG(INFO) << __func__;
    auto path = std::format("/data/vendor_de/{}/fpdata/", userId);
    uint64_t error = mDevice->setActiveGroup(mDevice, userId, path.c_str());
    if (error) {
        LOG(INFO) << "Failed to set active group: " << error;
    }
}

fingerprint_device_t* FingerprintEngine::openFingerprintHal() {
    const hw_module_t* hw_mdl = nullptr;

    LOG(INFO) << "Opening fingerprint hal library...";
    if (hw_get_module(FINGERPRINT_HARDWARE_MODULE_ID, &hw_mdl) != 0) {
        LOG(ERROR) << "Can't open fingerprint HW Module";
        return nullptr;
    }

    if (!hw_mdl) {
        LOG(ERROR) << "No valid fingerprint module";
        return nullptr;
    }

    auto module = reinterpret_cast<const fingerprint_module_t*>(hw_mdl);
    if (!module->common.methods->open) {
        LOG(ERROR) << "No valid open method";
        return nullptr;
    }

    hw_device_t* device = nullptr;
    if (module->common.methods->open(hw_mdl, nullptr, &device) != 0) {
        LOG(ERROR) << "Can't open fingerprint methods";
        return nullptr;
    }

    if (module->common.module_api_version != FINGERPRINT_MODULE_API_VERSION_2_1) {
        LOG(ERROR) << "Hardware version dosesn't match FINGERPRINT_MODULE_API_VERSION_2_1: " << module->common.module_api_version;
        return nullptr;
    }

    auto fp_device = reinterpret_cast<fingerprint_device_t*>(device);
    if (fp_device->set_notify(fp_device, Fingerprint::notify) != 0) {
        LOG(ERROR) << "Can't register fingerprint module callback";
        return nullptr;
    }

    return fp_device;
}

void FingerprintEngine::onAcquired(int32_t result, int32_t vendorCode) {
    LOG(INFO) << __func__;
    LOG(INFO) << " result: " << result << " vendorCode: " << vendorCode;
    if (result != FINGERPRINT_ACQUIRED_VENDOR) {
        setFingerStatus(false);
        if (result == FINGERPRINT_ACQUIRED_GOOD) setFodStatus(FOD_STATUS_OFF);
    } else if (vendorCode == 20 || vendorCode == 22) {
        /*
         * vendorCode = 20 waiting for fingerprint authentication
         * vendorCode = 22 waiting for fingerprint enroll
         */
        setFodStatus(FOD_STATUS_ON);
    } else if (vendorCode == 44) {
        /* vendorCode = 44 fingerprint scan failed */
        setFingerStatus(false);
    }
}

void FingerprintEngine::setFodStatus(int value) {
    set(FOD_STATUS_PATH, value);
}

void FingerprintEngine::setFingerStatus(bool pressed) {
    LOG(INFO) << __func__;
    mDevice->goodixExtCmd(mDevice, COMMAND_FOD_PRESS_STATUS, pressed ? PARAM_FOD_PRESSED : PARAM_FOD_RELEASED);
    mDevice->goodixExtCmd(mDevice, COMMAND_NIT, pressed ? PARAM_NIT_FOD : PARAM_NIT_NONE);

    set(DISP_PARAM_PATH,
        std::string(DISP_PARAM_LOCAL_HBM_MODE) + " " +
                (pressed ? DISP_PARAM_LOCAL_HBM_ON : DISP_PARAM_LOCAL_HBM_OFF));
}

template <typename T>
void FingerprintEngine::set(const std::string &path, const T &value){
    std::ofstream file(path);
    file << value;
}

void FingerprintEngine::generateChallengeImpl(ISessionCallback* /*cb*/) {
    LOG(INFO) << __func__;
    mDevice->generateChallenge(mDevice);
}

void FingerprintEngine::revokeChallengeImpl(ISessionCallback* /*cb*/, int64_t challenge) {
    LOG(INFO) << __func__;
    uint64_t error = mDevice->revokeChallenge(mDevice, challenge);
    if (error) {
        LOG(ERROR) << "Failed to revoke challenge=" << challenge
                    << " error=" << error;
    }
}

void FingerprintEngine::enrollImpl(ISessionCallback* cb,
                                       const keymaster::HardwareAuthToken& hat,
                                       const std::future<void>& /*cancel*/) {
    LOG(INFO) << __func__;

    hw_auth_token_t authToken;
    translate(hat, authToken);
    int error = mDevice->enroll(mDevice, &authToken);
    if (error){
        LOG(ERROR) << "enroll failed: " << error;
        cb->onError(Error::UNABLE_TO_PROCESS, error);
    }

}

void FingerprintEngine::authenticateImpl(ISessionCallback* cb, int64_t operationId,
                                             const std::future<void>& /*cancel*/) {
    LOG(INFO) << __func__;

    int error = mDevice->authenticate(mDevice, operationId);
    if (error) {
        LOG(ERROR) << "authenticate failed: " << error;
        cb->onError(Error::UNABLE_TO_PROCESS, error);
    }
}

void FingerprintEngine::detectInteractionImpl(ISessionCallback* cb,
                                                  const std::future<void>& /*cancel*/) {
    LOG(INFO) << __func__;

    auto detectInteractionSupported = Fingerprint::cfg().get<bool>("detect_interaction");
    if (!detectInteractionSupported) {
        LOG(ERROR) << "Detect interaction is not supported";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }
}

void FingerprintEngine::enumerateEnrollmentsImpl(ISessionCallback* cb) {
    LOG(INFO) << __func__;
    int error = mDevice->enumerate(mDevice);
    if (error) {
        LOG(ERROR) << "enumerate failed: " << error;
        cb->onError(Error::UNABLE_TO_PROCESS, error);
    }
}

void FingerprintEngine::removeEnrollmentsImpl(ISessionCallback * /*cb*/,
                                              const std::vector<int32_t> &enrollmentIds){
    LOG(INFO) << __func__;
    mDevice->remove(mDevice, enrollmentIds.data(), enrollmentIds.size());
}

void FingerprintEngine::getAuthenticatorIdImpl(ISessionCallback* /*cb*/) {
    LOG(INFO) << __func__;
    mDevice->getAuthenticatorId(mDevice);
}

void FingerprintEngine::invalidateAuthenticatorIdImpl(ISessionCallback* /*cb*/) {
    LOG(INFO) << __func__;
    mDevice->invalidateAuthenticatorId(mDevice);
}

void FingerprintEngine::resetLockoutImpl(ISessionCallback* cb,
                                             const keymaster::HardwareAuthToken& hat) {
    LOG(INFO) << __func__;
    if (hat.mac.empty()) {
        LOG(ERROR) << "Fail: hat in resetLockout()";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }
    clearLockout(cb);
    if (isLockoutTimerStarted) isLockoutTimerAborted = true;
}

void FingerprintEngine::clearLockout(ISessionCallback* cb, bool dueToTimeout) {
    cb->onLockoutCleared();
    mLockoutTracker.reset(dueToTimeout);
}

ndk::ScopedAStatus FingerprintEngine::onPointerDownImpl(int32_t /*pointerId*/, int32_t x,
                                                            int32_t y, float /*minor*/,
                                                            float /*major*/) {
    LOG(INFO) << __func__;
    // mDevice->onPointerDown(mDevice, pointerId, x, y, minor, major);
    mDevice->goodixExtCmd(mDevice, COMMAND_FOD_PRESS_X, x);
    mDevice->goodixExtCmd(mDevice, COMMAND_FOD_PRESS_Y, y);
    setFingerStatus(true);

    // verify whetehr touch coordinates/area matching sensor location ?
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FingerprintEngine::onPointerUpImpl(int32_t /*pointerId*/) {
    LOG(INFO) << __func__;

    // mDevice->onPointerUp(mDevice, pointerId);
    mDevice->goodixExtCmd(mDevice, COMMAND_FOD_PRESS_X, 0);
    mDevice->goodixExtCmd(mDevice, COMMAND_FOD_PRESS_Y, 0);
    setFingerStatus(false);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FingerprintEngine::onUiReadyImpl() {
    LOG(INFO) << __func__;
    return ndk::ScopedAStatus::ok();
}

SensorLocation FingerprintEngine::getSensorLocation() {
    SensorLocation location;

    auto loc = Fingerprint::cfg().get<std::string>("sensor_location");
    auto isValidStr = false;
    auto dim = Util::split(loc, ":");

    if (dim.size() < 3 or dim.size() > 4) {
        if (!loc.empty()) LOG(WARNING) << "Invalid sensor location input (x:y:radius):" + loc;
        return location;
    } else {
        int32_t x, y, r;
        std::string d = "";
        if (dim.size() >= 3) {
            isValidStr = ParseInt(dim[0], &x) && ParseInt(dim[1], &y) && ParseInt(dim[2], &r);
        }
        if (dim.size() >= 4) {
            d = dim[3];
        }
        if (isValidStr)
            location = {.sensorLocationX = x, .sensorLocationY = y, .sensorRadius = r, .display = d};

        return location;
    }
}

std::pair<AcquiredInfo, int32_t> FingerprintEngine::convertAcquiredInfo(int32_t code) {
    std::pair<AcquiredInfo, int32_t> res;
    if (code > FINGERPRINT_ACQUIRED_VENDOR_BASE) {
        res.first = AcquiredInfo::VENDOR;
        res.second = code - FINGERPRINT_ACQUIRED_VENDOR_BASE;
    } else {
        res.first = (AcquiredInfo)code;
        res.second = 0;
    }
    return res;
}

std::pair<Error, int32_t> FingerprintEngine::convertError(int32_t code) {
    std::pair<Error, int32_t> res;
    if (code > FINGERPRINT_ERROR_VENDOR_BASE) {
        res.first = Error::VENDOR;
        res.second = code - FINGERPRINT_ERROR_VENDOR_BASE;
    } else {
        res.first = (Error)code;
        res.second = 0;
    }
    return res;
}

bool FingerprintEngine::checkSensorLockout(ISessionCallback* cb) {
    LockoutTracker::LockoutMode lockoutMode = mLockoutTracker.getMode();
    if (lockoutMode == LockoutTracker::LockoutMode::kPermanent) {
        LOG(ERROR) << "Fail: lockout permanent";
        cb->onLockoutPermanent();
        isLockoutTimerAborted = true;
        return true;
    } else if (lockoutMode == LockoutTracker::LockoutMode::kTimed) {
        int64_t timeLeft = mLockoutTracker.getLockoutTimeLeft();
        LOG(ERROR) << "Fail: lockout timed " << timeLeft;
        cb->onLockoutTimed(timeLeft);
        if (isLockoutTimerSupported && !isLockoutTimerStarted) startLockoutTimer(timeLeft, cb);
        return true;
    }
    return false;
}

void FingerprintEngine::startLockoutTimer(int64_t timeout, ISessionCallback* cb) {
    LOG(INFO) << __func__;
    std::function<void(ISessionCallback*)> action =
            std::bind(&FingerprintEngine::lockoutTimerExpired, this, std::placeholders::_1);
    std::thread([timeout, action, cb]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        action(cb);
    }).detach();

    isLockoutTimerStarted = true;
}
void FingerprintEngine::lockoutTimerExpired(ISessionCallback* cb) {
    LOG(INFO) << __func__;
    if (!isLockoutTimerAborted) {
        clearLockout(cb, true);
    }
    isLockoutTimerStarted = false;
    isLockoutTimerAborted = false;
}
}  // namespace aidl::android::hardware::biometrics::fingerprint
