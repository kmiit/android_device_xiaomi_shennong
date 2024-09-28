/*
 * Copyright (C) 2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package com.xiaomi.settings.edgesuppression;

public class SuppressionRect {
    private int[] list = new int[8]; // Fixed size array for 8 integers
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

    @Override
    public String toString() {
        return "SuppressionRect{list=" + arrayToString(list) + ", type=" + type + ", position=" + position + ", topLeftX=" + topLeftX + ", topLeftY=" + topLeftY + ", bottomRightX=" + bottomRightX + ", bottomRightY=" + bottomRightY + ", time=" + 0 + ", node=" + 0 + '}';
    }

    // Method to populate the int[] list based on the instance variables
    public int[] getList() {
        list[0] = type;
        list[1] = position;
        list[2] = topLeftX;
        list[3] = topLeftY;
        list[4] = bottomRightX;
        list[5] = bottomRightY;
        list[6] = 0;  // Placeholder for time
        list[7] = 0;  // Placeholder for node
        return list;
    }

    // Utility method to convert int[] to a readable string
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
