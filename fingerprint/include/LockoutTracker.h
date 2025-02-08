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

#include <android/binder_to_string.h>
#include <stdint.h>
#include <string>

namespace aidl::android::hardware::biometrics::fingerprint {

class LockoutTracker {
  public:
    LockoutTracker() : mFailedCount(0), mFailedCountTimed(0) {}
    ~LockoutTracker() {}

    enum class LockoutMode : int8_t { kNone = 0, kTimed, kPermanent };

    void reset(bool dueToTimeout = false);
    LockoutMode getMode();
    void addFailedAttempt();
    int64_t getLockoutTimeLeft();
    inline std::string toString() const {
        std::ostringstream os;
        os << "----- LockoutTracker:: -----" << std::endl;
        os << "LockoutTracker::mFailedCount:" << mFailedCount;
        os << ", LockoutTracker::mCurrentMode:" << (int)mCurrentMode;
        os << std::endl;
        return os.str();
    }

  private:
    int32_t mFailedCount;
    int32_t mFailedCountTimed;
    int64_t mLockoutTimedStart;
    LockoutMode mCurrentMode;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
