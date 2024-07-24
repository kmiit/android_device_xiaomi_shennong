/*
 * Copyright (C) 2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package com.xiaomi.settings.edgesuppression;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.Switch;

import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;
import androidx.preference.PreferenceFragment;
import androidx.preference.PreferenceManager;
import androidx.preference.SeekBarPreference;

import com.android.settingslib.widget.MainSwitchPreference;
import com.android.settingslib.widget.OnMainSwitchChangeListener;

import com.xiaomi.settings.R;

public class EdgeSuppressionSettingsFragment extends PreferenceFragment implements OnPreferenceChangeListener,
        OnMainSwitchChangeListener {

    private static final String TAG = "XiaomiPartsEdgeSuppressionSettingsFragment";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    private EdgeSuppressionManager mEdgeSuppressionManager;
    private SharedPreferences mSharedPreferences;

    private MainSwitchPreference mSwitchBar;
    private SeekBarPreference mWidthPreference;

    private View mLeftView;
    private View mRightView;

    private FrameLayout.LayoutParams mLeftLayoutParams = new FrameLayout.LayoutParams(-1, -1, 51);
    private FrameLayout.LayoutParams mRightLayoutParams = new FrameLayout.LayoutParams(-1, -1, 53);

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mEdgeSuppressionManager = EdgeSuppressionManager.getInstance(getActivity().getApplicationContext());
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(getActivity().getApplicationContext());

        addPreferencesFromResource(R.xml.settings_edgesuppression);

        mSwitchBar = (MainSwitchPreference) findPreference("edgesuppression_enable");
        if (mSwitchBar != null) {
            mSwitchBar.addOnSwitchChangeListener(this);
        }

        mWidthPreference = (SeekBarPreference) findPreference("edgesuppression_width");
        if (mWidthPreference != null) {
            mWidthPreference.setUpdatesContinuously(true);
            mWidthPreference.setOnPreferenceChangeListener(this);
        }

        mEdgeSuppressionManager.handleEdgeSuppressionChange();
        setupEdgeSuppressionPreview(mSwitchBar.isChecked());
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        float value = (Float.parseFloat(newValue.toString()) + 20) / 100;
        updateEdgeSuppression(value);
        return true;
    }

    @Override
    public void onSwitchChanged(Switch switchView, boolean isChecked) {
        float value = isChecked ? (float) (mSharedPreferences.getInt("edgesuppression_width", 60) + 20) / 100 : 0.0f;
        updateEdgeSuppression(value);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    private void updateEdgeSuppression(float value) {
        int suppressionSize = mEdgeSuppressionManager.getSuppressionSize(false, value);
        mLeftLayoutParams.width = suppressionSize;
        mRightLayoutParams.width = suppressionSize;
        mLeftView.setLayoutParams(mLeftLayoutParams);
        mRightView.setLayoutParams(mRightLayoutParams);

        SharedPreferences.Editor editor = mSharedPreferences.edit();
        editor.putFloat("edgesuppression_width_value", value);
        editor.apply();

        mEdgeSuppressionManager.handleEdgeSuppressionChange();
    }

    private void setupEdgeSuppressionPreview(boolean enabled) {
        ViewGroup viewGroup = (ViewGroup) getActivity().getWindow().getDecorView();
        mLeftView = new View(getContext());
        mRightView = new View(getContext());

        int suppressionSize = enabled ? mEdgeSuppressionManager.getSuppressionSize(false,
                (float) (mSharedPreferences.getInt("edgesuppression_width", 60) + 20) / 100) : 0;
        WindowManager windowManager = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics metrics = new DisplayMetrics();
        windowManager.getDefaultDisplay().getRealMetrics(metrics);
        int screenHeight = Math.max(metrics.widthPixels, metrics.heightPixels) - 1;

        mLeftLayoutParams.height = screenHeight;
        mRightLayoutParams.height = screenHeight;
        mLeftLayoutParams.width = suppressionSize;
        mRightLayoutParams.width = suppressionSize;

        viewGroup.addView(mLeftView, mLeftLayoutParams);
        viewGroup.addView(mRightView, mRightLayoutParams);

        int restrictedColor = getResources().getColor(R.color.restricted_tip_area_color, null);
        mLeftView.setBackgroundColor(restrictedColor);
        mRightView.setBackgroundColor(restrictedColor);
    }
}
