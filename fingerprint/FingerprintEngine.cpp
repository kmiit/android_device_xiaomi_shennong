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
    : mWorkMode(WorkMode::kIdle),
      isLockoutTimerSupported(true) {
    if (mDevice) {
        LOG(INFO) << "Fingerprint HAL already opened";
    } else {
        for (auto& [module] : kModules) {
            std::string class_name;
            std::string class_module_id;

            auto parts = Util::split(module, ":");

            if (parts.size() == 2) {
                class_name = parts[0];
                class_module_id = parts[1];
            } else {
                class_name = module;
                class_module_id = FINGERPRINT_HARDWARE_MODULE_ID;
            }

            mDevice = openFingerprintHal(class_name.c_str(), class_module_id.c_str());
            if (!mDevice) {
                LOG(ERROR) << "Can't open HAL module, class: " << class_name.c_str() 
                    << ", module_id: " << class_module_id.c_str();
                continue;
            }
            LOG(INFO) << "Opened fingerprint HAL, class: " << class_name.c_str() 
                << ", module_id: " << class_module_id.c_str();
            break;
        }
        if (!mDevice) {
            LOG(ERROR) << "Can't open any fingerprint HAL module";
        }
    }
}

void FingerprintEngine::setActiveGroup(int userId) {
    auto path = std::format("/data/vendor_de/{}/fpdata/", userId);
    if (mDevice) {
        LOG(INFO) << "setActiveGroup";
        mDevice->setActiveGroup(mDevice, userId, path.c_str());
    } else {
        LOG(ERROR) << "Failed to set active group!";
    }
}

fingerprint_device_t* FingerprintEngine::openFingerprintHal(const char* class_name,
                                                      const char* module_id) {
    const hw_module_t* hw_mdl = nullptr;

    LOG(INFO) << "Opening fingerprint hal library...";
    if (hw_get_module_by_class(module_id, class_name, &hw_mdl) != 0) {
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
    BEGIN_OP(0);
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
    BEGIN_OP(0);
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
    BEGIN_OP(0);
    mDevice->generateChallenge(mDevice);
}

void FingerprintEngine::revokeChallengeImpl(ISessionCallback* /*cb*/, int64_t challenge) {
    BEGIN_OP(0);
    uint64_t error = mDevice->revokeChallenge(mDevice, challenge);
    if (error) {
        LOG(ERROR) << "Failed to revoke challenge=" << challenge
                    << " error=" << error;
    }
}

void FingerprintEngine::enrollImpl(ISessionCallback* cb,
                                       const keymaster::HardwareAuthToken& hat,
                                       const std::future<void>& cancel) {
    BEGIN_OP(0);

    // Do proper HAT verification in the real implementation.
    if (hat.mac.empty()) {
        LOG(ERROR) << "Fail: hat";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }

    waitForFingerDown(cb, cancel);

    updateContext(WorkMode::kEnroll, cb, const_cast<std::future<void>&>(cancel), 0, hat);
}

void FingerprintEngine::authenticateImpl(ISessionCallback* cb, int64_t operationId,
                                             const std::future<void>& cancel) {
    BEGIN_OP(0);

    waitForFingerDown(cb, cancel);

    updateContext(WorkMode::kAuthenticate, cb, const_cast<std::future<void>&>(cancel), operationId,
                  keymaster::HardwareAuthToken());
}

void FingerprintEngine::detectInteractionImpl(ISessionCallback* cb,
                                                  const std::future<void>& cancel) {
    BEGIN_OP(0);

    auto detectInteractionSupported = Fingerprint::cfg().get<bool>("detect_interaction");
    if (!detectInteractionSupported) {
        LOG(ERROR) << "Detect interaction is not supported";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }

    waitForFingerDown(cb, cancel);

    updateContext(WorkMode::kDetectInteract, cb, const_cast<std::future<void>&>(cancel), 0,
                  keymaster::HardwareAuthToken());
}

void FingerprintEngine::updateContext(WorkMode mode, ISessionCallback* cb,
                                          std::future<void>& cancel, int64_t operationId,
                                          const keymaster::HardwareAuthToken& hat) {
    mCancel = std::move(cancel);
    mWorkMode = mode;
    mCb = cb;
    mOperationId = operationId;
    mHat = hat;
}

void FingerprintEngine::fingerDownAction() {
    bool isTerminal = false;
    LOG(INFO) << __func__;
    switch (mWorkMode) {
        case WorkMode::kAuthenticate:
            isTerminal = onAuthenticateFingerDown(mCb, mOperationId, mCancel);
            break;
        case WorkMode::kEnroll:
            isTerminal = onEnrollFingerDown(mCb, mHat, mCancel);
            break;
        case WorkMode::kDetectInteract:
            isTerminal = onDetectInteractFingerDown(mCb, mCancel);
            break;
        default:
            LOG(WARNING) << "unexpected mode: on fingerDownAction(), " << (int)mWorkMode;
            break;
    }

    if (isTerminal) {
        mWorkMode = WorkMode::kIdle;
    }
}

bool FingerprintEngine::onEnrollFingerDown(ISessionCallback* cb,
                                               const keymaster::HardwareAuthToken& hat,
                                               const std::future<void>& cancel) {
    BEGIN_OP(getLatency(Fingerprint::cfg().getopt<OptIntVec>("operation_enroll_latency")));

    hw_auth_token_t authToken;
    translate(hat, authToken);
    int error = mDevice->enroll(mDevice, &authToken);
    if (error){
        LOG(ERROR) << "enroll failed: " << error;
        cb->onError(Error::UNABLE_TO_PROCESS, error);
    }
    if (shouldCancel(cancel)){
        LOG(ERROR) << "Fail: cancel";
        cb->onError(Error::CANCELED, 0 /* vendorCode */);
    }
    return true;
}

bool FingerprintEngine::onAuthenticateFingerDown(ISessionCallback* cb,
                                                     int64_t /* operationId */,
                                                     const std::future<void>& cancel) {
    BEGIN_OP(getLatency(Fingerprint::cfg().getopt<OptIntVec>("operation_authenticate_latency")));

    // got lockout?
    if (checkSensorLockout(cb)) {
        return LockoutTracker::LockoutMode::kPermanent == mLockoutTracker.getMode();
    }

    if (shouldCancel(cancel)) {
        LOG(ERROR) << "Fail: cancel";
        cb->onError(Error::CANCELED, 0 /* vendorCode */);
        return true;
    }

    int error = mDevice->authenticate(mDevice, operationId);
    if (error) {
        LOG(ERROR) << "authenticate failed: " << error;
    }
    return true;
}

bool FingerprintEngine::onDetectInteractFingerDown(ISessionCallback* cb,
                                                       const std::future<void>& cancel) {
    BEGIN_OP(getLatency(
            Fingerprint::cfg().getopt<OptIntVec>("operation_detect_interaction_latency")));

    int32_t duration =
            Fingerprint::cfg().get<std::int32_t>("operation_detect_interaction_duration");

    auto acquired = Fingerprint::cfg().get<std::string>("operation_detect_interaction_acquired");
    auto acquiredInfos = Util::parseIntSequence(acquired);
    int N = acquiredInfos.size();
    int64_t now = Util::getSystemNanoTime();

    if (N == 0) {
        LOG(ERROR) << "Fail to parse detect interaction acquired info: " + acquired;
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return true;
    }

    int i = 0;
    do {
        auto err = Fingerprint::cfg().get<std::int32_t>("operation_detect_interaction_error");
        if (err != 0) {
            LOG(ERROR) << "Fail: operation_detect_interaction_error";
            auto ec = convertError(err);
            cb->onError(ec.first, ec.second);
            return true;
        }

        if (shouldCancel(cancel)) {
            LOG(ERROR) << "Fail: cancel";
            cb->onError(Error::CANCELED, 0 /* vendorCode */);
            return true;
        }

        if (i < N) {
            auto ac = convertAcquiredInfo(acquiredInfos[i]);
            cb->onAcquired(ac.first, ac.second);
            i++;
        }
        SLEEP_MS(duration / N);
    } while (!Util::hasElapsed(now, duration));

    cb->onInteractionDetected();

    return true;
}

void FingerprintEngine::enumerateEnrollmentsImpl(ISessionCallback* /*cb*/) {
    BEGIN_OP(0);
    int error = mDevice->enumerate(mDevice);
    if (error) {
        LOG(ERROR) << "enumerate failed: " << error;
    }
}

void FingerprintEngine::removeEnrollmentsImpl(ISessionCallback * /*cb*/,
                                              const std::vector<int32_t> &enrollmentIds){
    BEGIN_OP(0);
    mDevice->remove(mDevice, enrollmentIds.data(), enrollmentIds.size());
}

void FingerprintEngine::getAuthenticatorIdImpl(ISessionCallback* /*cb*/) {
    BEGIN_OP(0);
    mDevice->getAuthenticatorId(mDevice);
}

void FingerprintEngine::invalidateAuthenticatorIdImpl(ISessionCallback* /*cb*/) {
    BEGIN_OP(0);
    mDevice->invalidateAuthenticatorId(mDevice);
}

void FingerprintEngine::resetLockoutImpl(ISessionCallback* cb,
                                             const keymaster::HardwareAuthToken& hat) {
    BEGIN_OP(0);
    if (hat.mac.empty()) {
        LOG(ERROR) << "Fail: hat in resetLockout()";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }
    clearLockout(cb);
    if (isLockoutTimerStarted) isLockoutTimerAborted = true;
}

void FingerprintEngine::clearLockout(ISessionCallback* cb, bool dueToTimeout) {
    Fingerprint::cfg().set<bool>("lockout", false);
    cb->onLockoutCleared();
    mLockoutTracker.reset(dueToTimeout);
}

ndk::ScopedAStatus FingerprintEngine::onPointerDownImpl(int32_t /*pointerId*/, int32_t /*x*/,
                                                            int32_t /*y*/, float /*minor*/,
                                                            float /*major*/) {
    BEGIN_OP(0);
    fingerDownAction();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FingerprintEngine::onPointerUpImpl(int32_t /*pointerId*/) {
    BEGIN_OP(0);
    mFingerIsDown = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus FingerprintEngine::onUiReadyImpl() {
    BEGIN_OP(0);
    return ndk::ScopedAStatus::ok();
}

bool FingerprintEngine::getSensorLocationConfig(SensorLocation& out) {
    auto loc = Fingerprint::cfg().get<std::string>("sensor_location");
    auto isValidStr = false;
    auto dim = Util::split(loc, ":");

    if (dim.size() < 3 or dim.size() > 4) {
        if (!loc.empty()) LOG(WARNING) << "Invalid sensor location input (x:y:radius):" + loc;
        return false;
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
            out = {.sensorLocationX = x, .sensorLocationY = y, .sensorRadius = r, .display = d};

        return isValidStr;
    }
}
SensorLocation FingerprintEngine::getSensorLocation() {
    SensorLocation location;

    if (getSensorLocationConfig(location)) {
        return location;
    } else {
        return defaultSensorLocation();
    }
}

SensorLocation FingerprintEngine::defaultSensorLocation() {
    return SensorLocation();
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

int32_t FingerprintEngine::getLatency(
        const std::vector<std::optional<std::int32_t>>& latencyIn) {
    int32_t res = DEFAULT_LATENCY;

    std::vector<int32_t> latency;
    for (auto x : latencyIn)
        if (x.has_value()) latency.push_back(*x);

    switch (latency.size()) {
        case 0:
            break;
        case 1:
            res = latency[0];
            break;
        case 2:
            res = getRandomInRange(latency[0], latency[1]);
            break;
        default:
            LOG(ERROR) << "ERROR: unexpected input of size " << latency.size();
            break;
    }

    return res;
}

int32_t FingerprintEngine::getRandomInRange(int32_t bound1, int32_t bound2) {
    std::uniform_int_distribution<int32_t> dist(std::min(bound1, bound2), std::max(bound1, bound2));
    return dist(mRandom);
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
    BEGIN_OP(0);
    std::function<void(ISessionCallback*)> action =
            std::bind(&FingerprintEngine::lockoutTimerExpired, this, std::placeholders::_1);
    std::thread([timeout, action, cb]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        action(cb);
    }).detach();

    isLockoutTimerStarted = true;
}
void FingerprintEngine::lockoutTimerExpired(ISessionCallback* cb) {
    BEGIN_OP(0);
    if (!isLockoutTimerAborted) {
        clearLockout(cb, true);
    }
    isLockoutTimerStarted = false;
    isLockoutTimerAborted = false;
}

void FingerprintEngine::waitForFingerDown(ISessionCallback* cb,
                                              const std::future<void>& cancel) {
    if (mFingerIsDown) {
        LOG(WARNING) << "waitForFingerDown: mFingerIsDown==true already!";
    }

    while (!mFingerIsDown) {
        if (shouldCancel(cancel)) {
            LOG(ERROR) << "waitForFingerDown, Fail: cancel";
            cb->onError(Error::CANCELED, 0 /* vendorCode */);
            return;
        }
        SLEEP_MS(10);
    }
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
