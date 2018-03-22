package com.dftc.libonvifvideo.service;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import com.dftc.libonvifvideo.OnvifVideoUtil;
import com.dftc.libonvifvideo.model.RTPConnection;
import com.dftc.libonvifvideo.model.RTPMac;
import com.dftc.libonvifvideo.model.RTPRtsp;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by xuqiqiang on 2018/3/6.
 * <p>
 * Description: IPC摄像头视频录制
 */
public class OnvifRecordService extends Service {
    public static final String ACTION = "com.rtsp.proxy.action.OnvifRecordService";
    public static final String KEY_DATA = "cameraList";
    private static final String TAG = "OnvifRecordService";
    private static final String VIDEO_FILE_PATH = "/sdcard/onvif/video";

    private RTPConnection mRTPConnection;
    private List<IPCDevice> mRecordIPCList = new ArrayList<>();

    public static void record(Context context, ArrayList<IPCDevice> cameraList) {
        Intent service = new Intent(context, OnvifRecordService.class);
        service.putParcelableArrayListExtra(OnvifRecordService.KEY_DATA,
                cameraList);
        context.startService(service);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        getVideoRecord();
    }

    private RTPConnection getVideoRecord() {
        if (mRTPConnection == null) {
            File dir = new File(VIDEO_FILE_PATH);
            if (dir.exists() || dir.mkdirs()) {
                mRTPConnection = OnvifVideoUtil
                        .getInstance().init(VIDEO_FILE_PATH);
            }
        }
        return mRTPConnection;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        ArrayList<IPCDevice> cameraList = intent.getParcelableArrayListExtra(KEY_DATA);
        if (cameraList != null)
            startRecord(cameraList);
        return START_STICKY;
    }

    private void startRecord(List<IPCDevice> cameraList) {
        if (IPCDeviceUtils.equals(cameraList, mRecordIPCList))
            return;
        Log.d(TAG, "startRecord");
        OnvifVideoUtil onvifVideoUtil = OnvifVideoUtil
                .getInstance();
        RTPConnection connection = getVideoRecord();

        List<IPCDevice> deleteList = new ArrayList<>();
        for (IPCDevice info : mRecordIPCList) {
            if (cameraList == null || !cameraList.contains(info)) {
                boolean deleteResult = onvifVideoUtil.delete(new RTPMac(connection, info.mac));
                Log.d(TAG, "delete " + info.mac + " " + deleteResult);
                if (deleteResult) {
                    deleteList.add(info);
                }
            }
        }
        mRecordIPCList.removeAll(deleteList);

        if (cameraList != null) {
            for (IPCDevice info : cameraList) {
                if (!mRecordIPCList.contains(info)) {
                    File dir = new File(VIDEO_FILE_PATH
                            + File.separator + info.mac.replace(":", "-"));
                    if (dir.exists() || dir.mkdirs()) {
                        int addResult = onvifVideoUtil.add(
                                new RTPRtsp(connection, info.mac, info.rtspAddr));
                        Log.d(TAG, "add " + info.mac + " " + addResult);
                        if (addResult != -1) {
                            mRecordIPCList.add(info);
                        }
                    }
                }
            }
        }
    }

}
