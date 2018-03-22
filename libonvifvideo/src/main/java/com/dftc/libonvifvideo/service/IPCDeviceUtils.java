package com.dftc.libonvifvideo.service;

import java.util.List;

/**
 * Created by xuqiqiang on 2018/1/23.
 * <p>
 * Description: 转换onvifscan进程获取的摄像头数据
 *
 * @see IPCDevice
 */
public class IPCDeviceUtils {

    public static boolean equals(List<IPCDevice> ipcList, List<IPCDevice> newIpcList) {
        return ipcList == null && newIpcList == null
                || !(ipcList == null || newIpcList == null || ipcList.size() != newIpcList.size()) && ipcList.equals(newIpcList);
    }
}
