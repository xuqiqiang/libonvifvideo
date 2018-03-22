package com.dftc.libonvifvideo.model;

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
public class RTPRtsp extends RTPMac {
    protected String rtsp;

    public RTPRtsp(RTPMac rtpMac, String rtsp) {
        super(rtpMac, rtpMac.mac);
        this.rtsp = rtsp;
    }

    public RTPRtsp(RTPConnection connection, String mac, String rtsp) {
        this(new RTPMac(connection, mac), rtsp);
    }

    public String getRtsp() {
        return rtsp;
    }

    public void setRtsp(String rtsp) {
        this.rtsp = rtsp;
    }
}
