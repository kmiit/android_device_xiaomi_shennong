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
                var name = "default";
                var fqName = vendor.xiaomi.hw.touchfeature.ITouchFeature.DESCRIPTOR + "/" + name;
                var binder = android.os.Binder.allowBlocking(android.os.ServiceManager.waitForDeclaredService(fqName));
                mTouchFeature = vendor.xiaomi.hw.touchfeature.ITouchFeature.Stub.asInterface(binder);
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
            if (params.valueArray != null) {
                touchfeature.setEdgeMode(0, params.mode, params.valueArray, params.valueArray.length);
            } else {
                touchfeature.setTouchMode(0, params.mode, params.value);
            }
        } catch (Exception e) {
            Log.e(TAG, "setTouchFeatureParams failed!", e);
        }
    }

    public static class TfParams {
        /* touchfeature parameters */
        final int mode, value;
        final int[] valueArray;

        // Constructor for single int value
        public TfParams(int mode, int value) {
            this.mode = mode;
            this.value = value;
            this.valueArray = null;
        }

        // Constructor for int array (int[])
        public TfParams(int mode, int[] valueArray) {
            this.mode = mode;
            this.value = 0;
            this.valueArray = valueArray;
        }

        @Override
        public String toString() {
            if (valueArray != null) {
                return "TouchFeatureParams(" + mode + ", " + arrayToString(valueArray) + ")";
            } else {
                return "TouchFeatureParams(" + mode + ", " + value + ")";
            }
        }

        // Helper method to convert int[] to String for logging purposes
        private String arrayToString(int[] array) {
            StringBuilder sb = new StringBuilder();
            sb.append("[");
            for (int i = 0; i < array.length; i++) {
                sb.append(array[i]);
                if (i < array.length - 1) {
                    sb.append(", ");
                }
            }
            sb.append("]");
            return sb.toString();
        }
    }
}
