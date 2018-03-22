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
public class RTPMac extends RTPConnection {
    protected String mac;

    public RTPMac(RTPConnection connection, String mac) {
        super(connection.hand);
        this.mac = mac;
    }

    public String getMac() {
        return mac;
    }

    public void setMac(String mac) {
        this.mac = mac;
    }
}
