/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <aidl/android/hardware/biometrics/fingerprint/BnSession.h>
#include <aidl/android/hardware/biometrics/fingerprint/ISessionCallback.h>

#include "FingerprintEngine.h"
#include "thread/WorkerThread.h"

#include "Legacy2Aidl.h"

namespace aidl::android::hardware::biometrics::fingerprint {

namespace common = aidl::android::hardware::biometrics::common;
namespace keymaster = aidl::android::hardware::keymaster;

void onClientDeath(void* cookie);

class Session : public BnSession {
  public:
    Session(int sensorId, int userId, std::shared_ptr<ISessionCallback> cb,
            FingerprintEngine* engine, WorkerThread* worker);

    ndk::ScopedAStatus generateChallenge() override;

    ndk::ScopedAStatus revokeChallenge(int64_t challenge) override;

    ndk::ScopedAStatus enroll(const keymaster::HardwareAuthToken& hat,
                              std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus authenticate(int64_t operationId,
                                    std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus detectInteraction(
            std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus enumerateEnrollments() override;

    ndk::ScopedAStatus removeEnrollments(const std::vector<int32_t>& enrollmentIds) override;

    ndk::ScopedAStatus getAuthenticatorId() override;

    ndk::ScopedAStatus invalidateAuthenticatorId() override;

    ndk::ScopedAStatus resetLockout(const keymaster::HardwareAuthToken& hat) override;

    ndk::ScopedAStatus close() override;

    ndk::ScopedAStatus onPointerDown(int32_t pointerId, int32_t x, int32_t y, float minor,
                                     float major) override;

    ndk::ScopedAStatus onPointerUp(int32_t pointerId) override;

    ndk::ScopedAStatus onUiReady() override;

    ndk::ScopedAStatus authenticateWithContext(
            int64_t operationId, const common::OperationContext& context,
            std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus enrollWithContext(
            const keymaster::HardwareAuthToken& hat, const common::OperationContext& context,
            std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus detectInteractionWithContext(
            const common::OperationContext& context,
            std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus onPointerDownWithContext(const PointerContext& context) override;

    ndk::ScopedAStatus onPointerUpWithContext(const PointerContext& context) override;

    ndk::ScopedAStatus onContextChanged(const common::OperationContext& context) override;

    ndk::ScopedAStatus onPointerCancelWithContext(const PointerContext& context) override;

    ndk::ScopedAStatus setIgnoreDisplayTouches(bool shouldIgnore) override;

    binder_status_t linkToDeath(AIBinder* binder);

    bool isClosed();

    void notify(const fingerprint_msg_t* msg);
  private:
    // The sensor and user IDs for which this session was created.
    int32_t mSensorId;
    int32_t mUserId;

    // Callback for talking to the framework. This callback must only be called from non-binder
    // threads to prevent nested binder calls and consequently a binder thread exhaustion.
    // Practically, it means that this callback should always be called from the worker thread.
    std::shared_ptr<ISessionCallback> mCb;

    // Module that communicates to the actual fingerprint hardware, keystore, TEE, etc. In real
    // life such modules typically consume a lot of memory and are slow to initialize. This is here
    // to showcase how such a module can be used within a Session without incurring the high
    // initialization costs every time a Session is constructed.
    FingerprintEngine* mEngine;

    // Worker thread that allows to schedule tasks for asynchronous execution.
    WorkerThread* mWorker;

    bool mIsClosed;
    // Binder death handler.
    AIBinder_DeathRecipient* mDeathRecipient;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
