/*
 * Copyright (C) 2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package com.xiaomi.settings.edgesuppression;

import android.content.Context;
import android.preference.PreferenceManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;

import java.util.ArrayList;
import java.util.Iterator;

import com.xiaomi.settings.R;
import com.xiaomi.settings.touch.TfWrapper;

public class EdgeSuppressionManager {

    private static final String TAG = "XiaomiPartsEdgeSuppressionManager";
    //private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);
    private static final boolean DEBUG = true;

    private Context mContext;

    private static EdgeSuppressionManager sInstance;

    private ArrayList<Integer> mSendList;
    private ArrayList<SuppressionRect> mRectList;
    private int mIndex;
    private int mScreenHeight;
    private int mScreenWidth;
    private int[] mAbsoluteLevel;
    private int[] mCorner;

    private enum Mode {
        CORNER(0),
        CONDITION(1),
        ABSOLUTE(2);

        private final int index;

        Mode(int i) {
            index = i;
        }
    }

    private EdgeSuppressionManager(Context context) {
        mContext = context.getApplicationContext();

        WindowManager windowManager = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics metrics = new DisplayMetrics();
        windowManager.getDefaultDisplay().getRealMetrics(metrics);
        mScreenWidth = Math.min(metrics.widthPixels, metrics.heightPixels) - 1;
        mScreenHeight = Math.max(metrics.widthPixels, metrics.heightPixels) - 1;

        mAbsoluteLevel = mContext.getResources().getIntArray(R.array.edge_suppresson_absolute);
        mCorner = mContext.getResources().getIntArray(R.array.edge_suppresson_corner);
        mRectList = new ArrayList<>(mContext.getResources().getInteger(R.integer.edge_suppresson_rect_size));
        mSendList = new ArrayList<>(mContext.getResources().getInteger(R.integer.edge_suppresson_send_size));
    }

    public static synchronized EdgeSuppressionManager getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new EdgeSuppressionManager(context);
        }
        return sInstance;
    }

    public ArrayList<Integer> handleEdgeSuppressionChange() {
        int rotation = ((WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay().getRotation();
        float width = PreferenceManager.getDefaultSharedPreferences(mContext).getFloat("edgesuppression_width_value", 0.8f);
        ArrayList<Integer> suppressionRect = getSuppressionRect(rotation, width);
        TfWrapper.setTouchFeature(new TfWrapper.TfParams(15, suppressionRect));
        return suppressionRect;
    }

    private ArrayList<Integer> getSuppressionRect(int rotation, float width) {
        mSendList.clear();
        if (mRectList.isEmpty()) {
            initRectList();
        }
        if (rotation == 1 || rotation == 3) {
            setRectPointForHorizontal(getSuppressionSize(true, width), Mode.ABSOLUTE.index);
            setRectPointForHorizontal(getSuppressionSize(false, width), Mode.CONDITION.index);
        } else {
            setRectPointForPortrait(getSuppressionSize(true, width), Mode.ABSOLUTE.index);
            setRectPointForPortrait(getSuppressionSize(false, width), Mode.CONDITION.index);
        }
        setCornerRectPoint(rotation);
        compileSendList();
        return mSendList;
    }

    private void setRectPointForHorizontal(int width, int modeIndex) {
        setRectValue(getCurrentRect(), modeIndex, 0, 0, 0, mScreenWidth, width);
        setRectValue(getCurrentRect(), modeIndex, 1, 0, mScreenHeight - width, mScreenWidth, mScreenHeight);
        setRectValue(getCurrentRect(), modeIndex, 2, 0, 0, width, mScreenHeight);
        setRectValue(getCurrentRect(), modeIndex, 3, mScreenWidth - width, 0, mScreenWidth, mScreenHeight);
    }

    private void setRectPointForPortrait(int width, int modeIndex) {
        setRectValue(getCurrentRect(), modeIndex, 0, 0, 0, 0, 0);
        setRectValue(getCurrentRect(), modeIndex, 1, 0, 0, 0, 0);
        setRectValue(getCurrentRect(), modeIndex, 2, 0, 0, width, mScreenHeight);
        setRectValue(getCurrentRect(), modeIndex, 3, mScreenWidth - width, 0, mScreenWidth, mScreenHeight);
    }

    private void setCornerRectPoint(int rotation) {
        switch (rotation) {
            case 0:
                setRectValue(getCurrentRect(), Mode.CORNER.index, 0, 0, 0, 0, 0);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 1, 0, 0, 0, 0);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 2, 0, mScreenHeight - mCorner[1], mCorner[0], mScreenHeight);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 3, mScreenWidth - mCorner[0], mScreenHeight - mCorner[1], mScreenWidth, mScreenHeight);
                break;
            case 1:
                setRectValue(getCurrentRect(), Mode.CORNER.index, 0, 0, 0, mCorner[2], mCorner[3]);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 1, 0, 0, 0, 0);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 2, 0, mScreenHeight - mCorner[3], mCorner[2], mScreenHeight);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 3, 0, 0, 0, 0);
                break;
            case 2:
                setRectValue(getCurrentRect(), Mode.CORNER.index, 0, 0, 0, mCorner[0], mCorner[1]);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 1, mScreenWidth - mCorner[0], 0, mScreenWidth, mCorner[1]);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 2, 0, 0, 0, 0);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 3, 0, 0, 0, 0);
                break;
            case 3:
                setRectValue(getCurrentRect(), Mode.CORNER.index, 0, 0, 0, 0, 0);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 1, mScreenWidth - mCorner[2], 0, mScreenWidth, mCorner[3]);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 2, 0, 0, 0, 0);
                setRectValue(getCurrentRect(), Mode.CORNER.index, 3, mScreenWidth - mCorner[2], mScreenHeight - mCorner[3], mScreenWidth, mScreenHeight);
                break;
        }
    }

    public int getSuppressionSize(boolean absolute, float width) {
        return (int) (width * (absolute ? 10 : 50));
    }

    private void initRectList() {
        int size = mContext.getResources().getInteger(R.integer.edge_suppresson_rect_size);
        for (int i = 0; i < size; i++) {
            mRectList.add(new SuppressionRect());
        }
    }

    private void setRectValue(SuppressionRect suppressionRect, int t, int p, int tx, int ty, int bx, int by) {
        suppressionRect.setValue(t, p, tx, ty, bx, by);
    }

    private void compileSendList() {
        for (SuppressionRect rect : mRectList) {
            mSendList.addAll(rect.getList());
        }
        resetIndex();
    }

    private void resetIndex() {
        mIndex = 0;
    }

    private SuppressionRect getCurrentRect() {
        return mRectList.get(mIndex++);
    }
}
