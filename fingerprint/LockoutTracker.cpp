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

#include "LockoutTracker.h"
#include <fingerprint.sysprop.h>
#include "Fingerprint.h"
#include "util/Util.h"

using namespace ::android::fingerprint::shennong;

namespace aidl::android::hardware::biometrics::fingerprint {

void LockoutTracker::reset(bool dueToTimeout) {
    LOG(INFO) << __func__;
    if (!dueToTimeout) {
        mFailedCount = 0;
    }
    mLockoutTimedStart = 0;
    mCurrentMode = LockoutMode::kNone;
}

void LockoutTracker::addFailedAttempt() {
    LOG(INFO) << __func__;
    mFailedCount++;
    if (mFailedCount >= LOCKOUT_PERMANENT_THRESHOLD) {
        mCurrentMode = LockoutMode::kPermanent;
    } else if (mFailedCount >= LOCKOUT_TIMED_THRESHOLD) {
        if (mCurrentMode == LockoutMode::kNone) {
            mCurrentMode = LockoutMode::kTimed;
            mLockoutTimedStart = Util::getSystemNanoTime();
        }
    }
}

LockoutTracker::LockoutMode LockoutTracker::getMode() {
    if (mCurrentMode == LockoutMode::kTimed) {
        if (Util::hasElapsed(mLockoutTimedStart, LOCKOUT_TIMED_DURATION)) {
            mCurrentMode = LockoutMode::kNone;
            mLockoutTimedStart = 0;
        }
    }

    return mCurrentMode;
}

int64_t LockoutTracker::getLockoutTimeLeft() {
    int64_t res = 0;

    if (mLockoutTimedStart > 0) {
        auto now = Util::getSystemNanoTime();
        auto elapsed = (now - mLockoutTimedStart) / 1000000LL;
        res = LOCKOUT_TIMED_DURATION - elapsed;
        LOG(INFO) << "elapsed=" << elapsed << " now = " << now
                  << " mLockoutTimedStart=" << mLockoutTimedStart << " res=" << res;
    }

    return res;
}
}  // namespace aidl::android::hardware::biometrics::fingerprint
