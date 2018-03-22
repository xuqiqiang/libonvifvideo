package com.dftc.libonvifvideo;


import com.dftc.libonvifvideo.model.RTPConnection;
import com.dftc.libonvifvideo.model.RTPMac;
import com.dftc.libonvifvideo.model.RTPPort;
import com.dftc.libonvifvideo.model.RTPRtsp;
import com.dftc.libonvifvideo.model.RTPVideoConfig;

/**
 * █████ ▒ █    ██  ▄████▄   ██ ▄█▀       ██████╗ ██╗   ██╗ ██████╗
 * ▓██   ▒ ██  ▓██▒▒██▀ ▀█   ██▄█▒        ██╔══██╗██║   ██║██╔════╝
 * ▒████ ░▓██  ▒██░▒▓█    ▄ ▓███▄░        ██████╔╝██║   ██║██║  ███╗
 * ░▓█▒  ░▓▓█  ░██░▒▓▓▄ ▄██▒▓██ █▄        ██╔══██╗██║   ██║██║   ██║
 * ░▒█░   ▒▒█████▓ ▒ ▓███▀ ░▒██▒ █▄       ██████╔╝╚██████╔╝╚██████╔╝
 * ▒  ░   ░▒▓▒ ▒ ▒ ░ ░▒ ▒  ░▒ ▒▒ ▓▒       ╚═════╝  ╚═════╝  ╚═════╝
 * ░      ░░▒░ ░ ░   ░  ▒   ░ ░▒ ▒░
 * ░  ░    ░░░ ░ ░ ░        ░ ░░ ░
 * ░      ░ ░      ░  ░
 * ░
 * Created by Administrator on 2017/10/13 0013
 */
public class OnvifVideoUtil {

    private static OnvifVideoUtil INSTANCE = null;

    private OnvifVideoUtil() {
    }

    public static OnvifVideoUtil getInstance() {
        if (INSTANCE == null) {
            INSTANCE = new OnvifVideoUtil();
        }
        return INSTANCE;
    }

    public interface RTPCallback<T> {
        void onSuccess(T data);
    }


    public void init(final String fileDir, final RTPCallback<RTPConnection> callback) {
        new Thread() {
            @Override
            public void run() {
                RTPConnection conn = init(fileDir);
                if (callback != null)
                    callback.onSuccess(conn);
            }
        }.start();
    }

    public void release(final RTPConnection hand, final RTPCallback callback) {
        new Thread() {
            @Override
            public void run() {
                release(hand);
                if (callback != null)
                    callback.onSuccess("");
            }
        }.start();
    }

    public void add(final RTPRtsp rtsp, final RTPCallback<Integer> callback) {
        new Thread() {
            @Override
            public void run() {
                int result = add(rtsp);
                if (callback != null) callback.onSuccess(result);
            }
        }.start();
    }

    public void delete(final RTPMac mac, final RTPCallback<Boolean> callback) {
        new Thread() {
            @Override
            public void run() {
                boolean result = delete(mac);
                if (callback != null) callback.onSuccess(result);
            }
        }.start();
    }

    public void sendData(final RTPPort port, final RTPCallback<Boolean> callback) {
        new Thread() {
            @Override
            public void run() {
                boolean result = sendData(port);
                if (callback != null) callback.onSuccess(result);
            }
        }.start();
    }

    public void stopData(final RTPMac mac, final RTPCallback<Boolean> callback) {
        new Thread() {
            @Override
            public void run() {
                boolean result = stopData(mac);
                if (callback != null) callback.onSuccess(result);
            }
        }.start();
    }

    public RTPConnection init(String fileDir) {

        return new RTPConnection(initVideo(fileDir));
    }

    public void release(RTPConnection hand) {
        try {
            if (hand != null && hand.getHand() > 0) {
                releaseVideo(hand.getHand());
            }
        } catch (NullPointerException e) {
            //do nothing
        }
    }

    public int add(RTPRtsp rtsp) {//1 已存在 0 成功 -1，失败
        return addIPC(rtsp.getHand(), rtsp.getMac(), rtsp.getRtsp());
    }

    public boolean delete(RTPMac mac) {
        return toBoolean(deleteIPC(mac.getHand(), mac.getMac()));
    }

    public boolean sendData(RTPPort port) {
        return toBoolean(sendIPCData(port.getHand(), port.getMac(), port.getPort()));
    }

    public boolean stopData(RTPMac mac) {
        return toBoolean(stopIPCData(mac.getHand(), mac.getMac()));
    }

    public void setVideoConfig(RTPVideoConfig config) {
        setVideoInfo(config.getConnection().getHand(), config.getTime(), config.getDay(), config.getSize());
    }

    public boolean toBoolean(int result) {
        if (result >= 0)
            return true;
        return false;
    }

    static {
        System.loadLibrary("OnvifVideo");
    }

    public native long initVideo(String dir);

    public native void releaseVideo(long hand);

    public native int addIPC(long hand, String mac, String rtsp);

    public native int deleteIPC(long hand, String mac);

    public native int sendIPCData(long hand, String mac, int port);

    public native int stopIPCData(long hand, String mac);

    public native void setVideoInfo(long hand, int time, int day, int size);

    public native void setLogLevel(int level);

    public static native long crc32(String fileName);

}
