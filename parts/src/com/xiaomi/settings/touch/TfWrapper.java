/*
 * Copyright (C) 2023-2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package com.xiaomi.settings.touch;

import android.os.IHwBinder.DeathRecipient;
import android.util.Log;
import java.util.ArrayList;

import vendor.xiaomi.hw.touchfeature.ITouchFeature;

public class TfWrapper {

    private static final String TAG = "XiaomiPartsTouchFeatureWrapper";

    private static ITouchFeature mTouchFeature;

    private static DeathRecipient mDeathRecipient = (cookie) -> {
        Log.d(TAG, "serviceDied");
        mTouchFeature = null;
    };

    public static ITouchFeature getTouchFeature() {
        if (mTouchFeature == null) {
            Log.d(TAG, "getTouchFeature: mTouchFeature=null");
            try {
                mTouchFeature = ITouchFeature.getService();
                mTouchFeature.asBinder().linkToDeath(mDeathRecipient, 0);
            } catch (Exception e) {
                Log.e(TAG, "getTouchFeature failed!", e);
            }
        }
        return mTouchFeature;
    }

    public static void setTouchFeature(TfParams params) {
        final ITouchFeature touchfeature = getTouchFeature();
        if (touchfeature == null) {
            Log.e(TAG, "setTouchFeatureParams: touchfeature is null!");
            return;
        }
        Log.d(TAG, "setTouchFeatureParams: " + params);
        try {
            if (params.valueList != null) {
                touchfeature.setModeLongValue(0, params.mode, params.valueList.size(), params.valueList);
            } else {
                touchfeature.setModeValue(0, params.mode, params.value);
            }
        } catch (Exception e) {
            Log.e(TAG, "setTouchFeatureParams failed!", e);
        }
    }

    public static class TfParams {
        /* touchfeature parameters */
        final int mode, value;
        final ArrayList<Integer> valueList;

        public TfParams(int mode, int value) {
            this.mode = mode;
            this.value = value;
            this.valueList = null;
        }

        public TfParams(int mode, ArrayList<Integer> valueList) {
            this.mode = mode;
            this.value = 0;
            this.valueList = valueList;
        }

        public String toString() {
            if (valueList != null) {
                return "TouchFeatureParams(" + mode + ", " + valueList + ")";
            } else {
                return "TouchFeatureParams(" + mode + ", " + value + ")";
            }
        }
    }
}
