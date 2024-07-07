/*
 * Copyright (C) 2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package com.xiaomi.settings.edgesuppression;

import java.util.ArrayList;

public class SuppressionRect {
    private ArrayList<Integer> list = new ArrayList<>();
    private int bottomRightX;
    private int bottomRightY;
    private int position;
    private int topLeftX;
    private int topLeftY;
    private int type;

    public void setType(int i) {
        type = i;
    }

    public void setPosition(int i) {
        position = i;
    }

    public void setTopLeftY(int i) {
        topLeftY = i;
    }

    public void setTopLeftX(int i) {
        topLeftX = i;
    }

    public void setBottomRightX(int i) {
        bottomRightX = i;
    }

    public void setBottomRightY(int i) {
        bottomRightY = i;
    }

    public void setValue(int t, int p, int tx, int ty, int bx, int by) {
        type = t;
        position = p;
        topLeftX = tx;
        topLeftY = ty;
        bottomRightX = bx;
        bottomRightY = by;
    }

    public String toString() {
        return "SuppressionRect{list=" + list + ", type=" + type + ", position=" + position + ", topLeftX=" + topLeftX + ", topLeftY=" + topLeftY + ", bottomRightX=" + bottomRightX + ", bottomRightY=" + bottomRightY + ", time=" + 0 + ", node=" + 0 + '}';
    }

    public ArrayList<Integer> getList() {
        if (list.size() != 0) {
            list.clear();
        }
        list.add(Integer.valueOf(type));
        list.add(Integer.valueOf(position));
        list.add(Integer.valueOf(topLeftX));
        list.add(Integer.valueOf(topLeftY));
        list.add(Integer.valueOf(bottomRightX));
        list.add(Integer.valueOf(bottomRightY));
        list.add(Integer.valueOf(0));
        list.add(Integer.valueOf(0));
        return list;
    }
}
