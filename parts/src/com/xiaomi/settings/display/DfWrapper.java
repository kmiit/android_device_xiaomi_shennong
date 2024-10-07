/*
 * Copyright (C) 2023-2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package com.xiaomi.settings.display;

import android.os.IBinder;
import android.os.ServiceManager;
import android.util.Log;
import vendor.xiaomi.hardware.displayfeature_aidl.IDisplayFeature;

public class DfWrapper {

    private static final String TAG = "XiaomiPartsDisplayFeatureWrapper";

    private static IDisplayFeature mDisplayFeature;

    private static IBinder.DeathRecipient mDeathRecipient = new IBinder.DeathRecipient() {
        @Override
        public void binderDied() {
            Log.d(TAG, "serviceDied");
            mDisplayFeature = null;
        }
    };

    public static IDisplayFeature getDisplayFeature() {
        if (mDisplayFeature == null) {
            Log.d(TAG, "getDisplayFeature: mDisplayFeature=null");
            try {
                String name = "default";
                String fqName = IDisplayFeature.DESCRIPTOR + "/" + name;
                IBinder binder = android.os.Binder.allowBlocking(ServiceManager.waitForDeclaredService(fqName));
                mDisplayFeature = IDisplayFeature.Stub.asInterface(binder);
                mDisplayFeature.asBinder().linkToDeath(mDeathRecipient, 0);
            } catch (Exception e) {
                Log.e(TAG, "getDisplayFeature failed!", e);
            }
        }
        return mDisplayFeature;
    }

    public static void setDisplayFeature(DfParams params) {
        final IDisplayFeature displayFeature = getDisplayFeature();
        if (displayFeature == null) {
            Log.e(TAG, "setDisplayFeatureParams: displayFeature is null!");
            return;
        }
        Log.d(TAG, "setDisplayFeatureParams: " + params);
        try {
            displayFeature.setFeature(0, params.mode, params.value, params.cookie);
        } catch (Exception e) {
            Log.e(TAG, "setDisplayFeatureParams failed!", e);
        }
    }

    public static class DfParams {
        /* displayfeature parameters */
        final int mode, value, cookie;

        public DfParams(int mode, int value, int cookie) {
            this.mode = mode;
            this.value = value;
            this.cookie = cookie;
        }

        public String toString() {
            return "DisplayFeatureParams(" + mode + ", " + value + ", " + cookie + ")";
        }
    }
}
