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

#include "Session.h"

#include <android-base/logging.h>

#include "util/CancellationSignal.h"

#undef LOG_TAG
#define LOG_TAG "FingerprintHalSession"

namespace aidl::android::hardware::biometrics::fingerprint {

void onClientDeath(void* cookie) {
    LOG(INFO) << "FingerprintService has died";
    Session* session = static_cast<Session*>(cookie);
    if (session && !session->isClosed()) {
        session->close();
    }
}

Session::Session(int sensorId, int userId, std::shared_ptr<ISessionCallback> cb,
                 FingerprintEngine* engine, WorkerThread* worker)
    : mSensorId(sensorId),
      mUserId(userId),
      mCb(std::move(cb)),
      mEngine(engine),
      mWorker(worker) {
    CHECK_GE(mSensorId, 0);
    CHECK_GE(mUserId, 0);
    CHECK(mEngine);
    CHECK(mWorker);
    CHECK(mCb);

    mDeathRecipient = AIBinder_DeathRecipient_new(onClientDeath);
    mEngine->setActiveGroup(mUserId);
}

binder_status_t Session::linkToDeath(AIBinder* binder) {
    return AIBinder_linkToDeath(binder, mDeathRecipient, this);
}

bool Session::isClosed() {
    return mIsClosed;
}

ndk::ScopedAStatus Session::generateChallenge() {
    LOG(INFO) << "generateChallenge";

    mWorker->schedule(Callable::from([this] {
        mEngine->generateChallengeImpl(mCb.get());
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::revokeChallenge(int64_t challenge) {
    LOG(INFO) << "revokeChallenge";

    mWorker->schedule(Callable::from([this, challenge] {
        mEngine->revokeChallengeImpl(mCb.get(), challenge);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enroll(const keymaster::HardwareAuthToken& hat,
                                   std::shared_ptr<common::ICancellationSignal>* out) {
    LOG(INFO) << "enroll";

    std::promise<void> cancellationPromise;
    auto cancFuture = cancellationPromise.get_future();

    mWorker->schedule(Callable::from([this, hat, cancFuture = std::move(cancFuture)] {
        if (shouldCancel(cancFuture)) {
            mCb->onError(Error::CANCELED, 0 /* vendorCode */);
        } else {
            mEngine->enrollImpl(mCb.get(), hat, cancFuture);
        }
    }));

    *out = SharedRefBase::make<CancellationSignal>(std::move(cancellationPromise));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticate(int64_t operationId,
                                         std::shared_ptr<common::ICancellationSignal>* out) {
    LOG(INFO) << "authenticate";

    std::promise<void> cancPromise;
    auto cancFuture = cancPromise.get_future();

    mWorker->schedule(Callable::from([this, operationId, cancFuture = std::move(cancFuture)] {
        if (shouldCancel(cancFuture)) {
            mCb->onError(Error::CANCELED, 0 /* vendorCode */);
        } else {
            mEngine->authenticateImpl(mCb.get(), operationId, cancFuture);
        }
    }));

    *out = SharedRefBase::make<CancellationSignal>(std::move(cancPromise));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::detectInteraction(std::shared_ptr<common::ICancellationSignal>* out) {
    LOG(INFO) << "detectInteraction";

    std::promise<void> cancellationPromise;
    auto cancFuture = cancellationPromise.get_future();

    mWorker->schedule(Callable::from([this, cancFuture = std::move(cancFuture)] {
        if (shouldCancel(cancFuture)) {
            mCb->onError(Error::CANCELED, 0 /* vendorCode */);
        } else {
            mEngine->detectInteractionImpl(mCb.get(), cancFuture);
        }
    }));

    *out = SharedRefBase::make<CancellationSignal>(std::move(cancellationPromise));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enumerateEnrollments() {
    LOG(INFO) << "enumerateEnrollments";

    mWorker->schedule(Callable::from([this] {
        mEngine->enumerateEnrollmentsImpl(mCb.get());
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::removeEnrollments(const std::vector<int32_t>& enrollmentIds) {
    LOG(INFO) << "removeEnrollments, size:" << enrollmentIds.size();

    mWorker->schedule(Callable::from([this, enrollmentIds] {
        mEngine->removeEnrollmentsImpl(mCb.get(), enrollmentIds);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getAuthenticatorId() {
    LOG(INFO) << "getAuthenticatorId";

    mWorker->schedule(Callable::from([this] {
        mEngine->getAuthenticatorIdImpl(mCb.get());
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::invalidateAuthenticatorId() {
    LOG(INFO) << "invalidateAuthenticatorId";

    mWorker->schedule(Callable::from([this] {
        mEngine->invalidateAuthenticatorIdImpl(mCb.get());
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::resetLockout(const keymaster::HardwareAuthToken& hat) {
    LOG(INFO) << "resetLockout";

    mWorker->schedule(Callable::from([this, hat] {
        mEngine->resetLockoutImpl(mCb.get(), hat);
    }));

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::close() {
    LOG(INFO) << "close";
    // TODO(b/166800618): call enterIdling from the terminal callbacks and restore this check.
    // CHECK(mCurrentState == SessionState::IDLING) << "Can't close a non-idling session.
    // Crashing.";
    mIsClosed = true;
    mCb->onSessionClosed();
    AIBinder_DeathRecipient_delete(mDeathRecipient);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerDown(int32_t pointerId, int32_t x, int32_t y, float minor,
                                          float major) {
    LOG(INFO) << "onPointerDown";
    mWorker->schedule(Callable::from([this, pointerId, x, y, minor, major] {
        bool isLockout = mEngine->checkSensorLockout(mCb.get());
        if (!isLockout) mEngine->onPointerDownImpl(pointerId, x, y, minor, major);
    }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerUp(int32_t pointerId) {
    LOG(INFO) << "onPointerUp";
    mWorker->schedule(Callable::from([this, pointerId] {
        mEngine->onPointerUpImpl(pointerId);
    }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onUiReady() {
    LOG(INFO) << "onUiReady";
    mWorker->schedule(Callable::from([this] {
        mEngine->onUiReadyImpl();
    }));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticateWithContext(
        int64_t operationId, const common::OperationContext& /*context*/,
        std::shared_ptr<common::ICancellationSignal>* out) {
    return authenticate(operationId, out);
}

ndk::ScopedAStatus Session::enrollWithContext(const keymaster::HardwareAuthToken& hat,
                                              const common::OperationContext& /*context*/,
                                              std::shared_ptr<common::ICancellationSignal>* out) {
    return enroll(hat, out);
}

ndk::ScopedAStatus Session::detectInteractionWithContext(
        const common::OperationContext& /*context*/,
        std::shared_ptr<common::ICancellationSignal>* out) {
    return detectInteraction(out);
}

ndk::ScopedAStatus Session::onPointerDownWithContext(const PointerContext& context) {
    return onPointerDown(context.pointerId, context.x, context.y, context.minor, context.major);
}

ndk::ScopedAStatus Session::onPointerUpWithContext(const PointerContext& context) {
    return onPointerUp(context.pointerId);
}

ndk::ScopedAStatus Session::onContextChanged(const common::OperationContext& /*context*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerCancelWithContext(const PointerContext& /*context*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::setIgnoreDisplayTouches(bool /*shouldIgnore*/) {
    return ndk::ScopedAStatus::ok();
}

void Session::notify(const fingerprint_msg_t* msg) {
    // const uint64_t devId = reinterpret_cast<uint64_t>(mDevice);
    switch (msg->type) {
        case FINGERPRINT_ERROR: {
            std::pair<Error, int32_t> result = mEngine->convertError(msg->data.error);
            LOG(INFO) << "onError(" << static_cast<int>(result.first) << ", " << result.second << ")";
            mCb->onError(result.first, result.second);
        } break;
        case FINGERPRINT_ACQUIRED: {
            std::pair<AcquiredInfo, int32_t> result =
                    mEngine->convertAcquiredInfo(msg->data.acquired.acquired_info);
            LOG(INFO) << "onAcquired(" << static_cast<int>(result.first) << ", " << result.second << ")";
            mEngine->onAcquired(static_cast<int32_t>(result.first), result.second);
            // don't process vendor messages further since frameworks try to disable
            // udfps display mode on vendor acquired messages but our sensors send a
            // vendor message during processing...
            if (result.first != AcquiredInfo::VENDOR) {
                mCb->onAcquired(result.first, result.second);
            }
        } break;
        case FINGERPRINT_TEMPLATE_ENROLLING: {
            LOG(INFO) << "onEnrollResult(fid=" << msg->data.enroll.fid
                        << ", rem=" << msg->data.enroll.samples_remaining << ")";
            mCb->onEnrollmentProgress(msg->data.enroll.fid,
                                      msg->data.enroll.samples_remaining);
        } break;
        case FINGERPRINT_TEMPLATE_REMOVED: {
            LOG(INFO) << "onRemove(fid=" << msg->data.removed.fid
                        << ", rem=" << msg->data.removed.remaining_templates << ")";
            std::vector<int> enrollments;
            enrollments.push_back(msg->data.removed.fid);
            mCb->onEnrollmentsRemoved(enrollments);
        } break;
        case FINGERPRINT_AUTHENTICATED: {
            LOG(INFO) << "onAuthenticated(fid=" << msg->data.authenticated.finger.fid << ")";
            if (msg->data.authenticated.finger.fid != 0) {
                const hw_auth_token_t hat = msg->data.authenticated.hat;
                keymaster::HardwareAuthToken authToken;
                translate(hat, authToken);

                mCb->onAuthenticationSucceeded(msg->data.authenticated.finger.fid, authToken);
                mEngine->mLockoutTracker.reset(true);
            } else {
                mCb->onAuthenticationFailed();
                mEngine->mLockoutTracker.addFailedAttempt();
                mEngine->checkSensorLockout(mCb.get());
            }
            mEngine->onPointerUpImpl(0);
        } break;
        case FINGERPRINT_TEMPLATE_ENUMERATING: {
            LOG(INFO) << "onEnumerate(fid=" << msg->data.enumerated.fid 
                        << ", rem=" << msg->data.enumerated.remaining_templates << ")";
            static std::vector<int> enrollments;
            enrollments.push_back(msg->data.enumerated.fid);
            if (msg->data.enumerated.remaining_templates == 0) {
                mCb->onEnrollmentsEnumerated(enrollments);
                enrollments.clear();
            }
        } break;
        case FINGERPRINT_CHALLENGE_GENERATED: {
            int64_t challenge = msg->data.extend.data;
            LOG(INFO) << "onChallengeGenerated: " << challenge;
            mCb->onChallengeGenerated(challenge);
        } break;
        case FINGERPRINT_CHALLENGE_REVOKED: {
            int64_t challenge = msg->data.extend.data;
            LOG(INFO) << "onChallengeRevoked: " << challenge;
            mCb->onChallengeRevoked(challenge);
        } break;
        case FINGERPRINT_AUTHENTICATOR_ID_RETRIEVED: {
            int auth_id = msg->data.extend.data;
            LOG(INFO) << "onAuthenticatorIDRetrieved: " << auth_id;
            mEngine->onPointerUpImpl(0);
            mCb->onAuthenticatorIdRetrieved(auth_id);
        } break;
        case FINGERPRINT_AUTHENTICATOR_ID_INVALIDATED: {
            int64_t new_auth_id = msg->data.extend.data;
            LOG(INFO) <<"onAuthenticatorIDInvalidated, new auth id: " << new_auth_id;
            mCb->onAuthenticatorIdInvalidated(new_auth_id);
        } break;
        default:
            LOG(ERROR) << "received unknown message: " << msg->type;
    }
}
}  // namespace aidl::android::hardware::biometrics::fingerprint
