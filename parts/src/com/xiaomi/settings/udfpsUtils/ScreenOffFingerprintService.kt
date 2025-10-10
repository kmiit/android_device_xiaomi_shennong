package com.xiaomi.settings.udfpsUtils

import android.app.Service
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.database.ContentObserver
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.UserHandle
import android.provider.Settings
import android.util.Log
import android.view.Display

import com.xiaomi.settings.touch.TfWrapper.setTouchFeature
import com.xiaomi.settings.touch.TfWrapper.TfParams

class ScreenOffFingerprintService : Service() {
    private val GUESTURE_LONGPRESS_MODE = 10

    private val TAG = "ScreenOffFpService"

    private var mScreenOffUnlockUdfpsEnabled = 0
    private var mScreenOn: Boolean = false

    private val mSettingObserver = object : ContentObserver(Handler(Looper.getMainLooper())) {
        override fun onChange(selfChange: Boolean) {
            Log.d(TAG, "SettingObserver: onChange. Self change: $selfChange")
            updateScreenOffFODStatus()
        }
    }

    private val mScreenStateReceiver = object : BroadcastReceiver() {
        fun register() {
            Log.d(TAG, "ScreenStateReceiver: register")
            registerReceiver(this, IntentFilter(Intent.ACTION_SCREEN_ON))
            registerReceiver(this, IntentFilter(Intent.ACTION_SCREEN_OFF))
        }

        override fun onReceive(context: Context, intent: Intent) {
            mScreenOn = getDisplay().state == Display.STATE_ON
            updateTouchMode()
        }
    }

    override fun onCreate() {
        super.onCreate()
        Log.i(TAG, "Service onCreate")
        contentResolver.registerContentObserver(
            Settings.Secure.getUriFor(Settings.Secure.SCREEN_OFF_UNLOCK_UDFPS_ENABLED),
            false,
            mSettingObserver,
            UserHandle.USER_CURRENT
        )
        mScreenStateReceiver.register()
        updateScreenOffFODStatus()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.i(TAG, "Service onStartCommand")
        return START_STICKY
    }

    override fun onDestroy() {
        Log.i(TAG, "Service onDestroy")
        contentResolver.unregisterContentObserver(mSettingObserver)
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? { return null }

    private fun updateScreenOffFODStatus() {
        mScreenOffUnlockUdfpsEnabled = Settings.Secure.getIntForUser(
            contentResolver,
            Settings.Secure.SCREEN_OFF_UNLOCK_UDFPS_ENABLED,
            0,
            UserHandle.USER_CURRENT
        )
    }

    private fun updateTouchMode() {
        val fodTfParam = TfParams(GUESTURE_LONGPRESS_MODE, if (mScreenOffUnlockUdfpsEnabled == 1 && !mScreenOn) 1 else 0)
        setTouchFeature(fodTfParam)
    }
}
