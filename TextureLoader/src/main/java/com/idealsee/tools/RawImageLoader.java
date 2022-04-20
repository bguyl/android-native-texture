package com.idealsee.tools;


import java.io.File;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.util.Log;

public class RawImageLoader implements Runnable {

    private static final String TAG = "RawImageLoader";
    private float targetHeight, targetWidth;
    private byte[] ImageData = new byte[]{};

    public static Bitmap makeBitmap(byte[] jpegData, int maxNumOfPixels) {
        try {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length, options);
            if (options.mCancel || options.outWidth == -1 || options.outHeight == -1) {
                return null;
            }
            options.inSampleSize = computeSampleSize(options, -1, maxNumOfPixels);
            options.inJustDecodeBounds = false;

            options.inDither = false;
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            return BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length, options);
        } catch (OutOfMemoryError ex) {
            Log.e(TAG, "Got oom exception ", ex);
            return null;
        }
    }


    public static int computeSampleSize(BitmapFactory.Options options, int minSideLength, int maxNumOfPixels) {
        int initialSize = computeInitialSampleSize(options, minSideLength, maxNumOfPixels);

        int roundedSize;
        if (initialSize <= 8) {
            roundedSize = 1;
            while (roundedSize < initialSize) {
                roundedSize <<= 1;
            }
        } else {
            roundedSize = (initialSize + 7) / 8 * 8;
        }

        return roundedSize;
    }

    private static int computeInitialSampleSize(BitmapFactory.Options options, int minSideLength, int maxNumOfPixels) {
        double w = options.outWidth;
        double h = options.outHeight;

        int lowerBound = (maxNumOfPixels < 0) ? 1 : (int) Math.ceil(Math.sqrt(w * h / maxNumOfPixels));
        int upperBound = (minSideLength < 0) ? 128
                : (int) Math.min(Math.floor(w / minSideLength), Math.floor(h / minSideLength));

        if (upperBound < lowerBound) {
            // return the larger one when there is no overlapping zone.
            return lowerBound;
        }

        if (maxNumOfPixels < 0 && minSideLength < 0) {
            return 1;
        } else if (minSideLength < 0) {
            return lowerBound;
        } else {
            return upperBound;
        }
    }

    @Override
    public void run() {
        // TODO Auto-generated method stub
        Log.v(TAG, "Thread load Image run");
        Bitmap normalImage = null;
        try {

            Bitmap rawImage = makeBitmap(ImageData, Integer.MAX_VALUE);
            normalImage = NormalizeBitmap(rawImage);
        } catch (Exception e) {
            Log.e(TAG, "Load Bitmap Error : " + e);
        }

        Log.v(TAG, "Thread load image finish! ");

        if (mImageFileLoadFinish != null) {
            mImageFileLoadFinish.OnLoadCompleted(normalImage);
        }
    }

    public interface IImageFileLoadFinish {
        void OnLoadCompleted(Bitmap mImage);
    }

    private IImageFileLoadFinish mImageFileLoadFinish;

    public RawImageLoader(byte[] imageData, IImageFileLoadFinish callback, float width, float height)
    {
        ImageData = imageData;
        mImageFileLoadFinish = callback;
        targetWidth = width;
        targetHeight = height;
    }

    private Bitmap NormalizeBitmap(Bitmap raw) {
        Matrix scaleMatrix = new Matrix();
        float rawHeight = raw.getHeight();
        float rawWidth = raw.getWidth();
        Bitmap target;
        if (rawHeight != targetHeight || rawWidth != targetWidth) {
            scaleMatrix.postScale(targetWidth / rawWidth, -targetHeight / rawHeight);
            target = Bitmap.createBitmap(raw, 0, 0, (int)rawWidth, (int)rawHeight, scaleMatrix, false);
        } else {
            target = raw;
        }
        return target;
    }

}
