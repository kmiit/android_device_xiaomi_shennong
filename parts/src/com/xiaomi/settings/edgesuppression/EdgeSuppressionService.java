/*
 * Copyright (C) 2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package com.xiaomi.settings.edgesuppression;

import android.app.Service;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

public class EdgeSuppressionService extends Service {

    private static final String TAG = "XiaomiPartsEdgeSuppressionService";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    private EdgeSuppressionManager mEdgeSuppressionManager;

    @Override
    public void onCreate() {
        if (Build.SKU.equals("aurora")) {
            if (DEBUG) Log.d(TAG, "Creating service");
            super.onCreate();
            mEdgeSuppressionManager = EdgeSuppressionManager.getInstance(getApplicationContext());
            getPackageManager().setComponentEnabledSetting(
                    new ComponentName(this, "com.xiaomi.settings.edgesuppression.EdgeSuppressionSettingsActivity"),
                    PackageManager.COMPONENT_ENABLED_STATE_ENABLED, PackageManager.DONT_KILL_APP);
        } else {
            if (DEBUG) Log.d(TAG, "Stopping service, not supported on this device");
            getPackageManager().setComponentEnabledSetting(
                    new ComponentName(this, "com.xiaomi.settings.edgesuppression.EdgeSuppressionSettingsActivity"),
                    PackageManager.COMPONENT_ENABLED_STATE_DISABLED, PackageManager.DONT_KILL_APP);
            stopSelf();
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (DEBUG) Log.d(TAG, "onStartCommand");
        mEdgeSuppressionManager.handleEdgeSuppressionChange();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "onDestroy");
        super.onDestroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (DEBUG) Log.d(TAG, "onConfigurationChanged");
        mEdgeSuppressionManager.handleEdgeSuppressionChange();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
