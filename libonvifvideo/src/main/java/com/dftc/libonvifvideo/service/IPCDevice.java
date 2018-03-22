package com.dftc.libonvifvideo.service;

import android.os.Parcel;
import android.os.Parcelable;

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
public class IPCDevice implements Parcelable {
    public static final Creator<IPCDevice> CREATOR = new Creator<IPCDevice>() {
        @Override
        public IPCDevice createFromParcel(Parcel parcel) {
            return new IPCDevice(parcel);
        }

        @Override
        public IPCDevice[] newArray(int i) {
            return new IPCDevice[i];
        }
    };
    public String mac;
    public String rtspAddr;

    public IPCDevice() {
    }

    public IPCDevice(String mac, String rtspAddr) {
        this.mac = mac;
        this.rtspAddr = rtspAddr;
    }

    public IPCDevice(Parcel in) {
        this.mac = in.readString();
        this.rtspAddr = in.readString();
    }

    @Override
    public boolean equals(Object o) {
        if (o instanceof IPCDevice) {
            return ((IPCDevice) o).mac.equals(this.mac);
        }
        return super.equals(o);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeString(mac);
        parcel.writeString(rtspAddr);
    }

    @Override
    public String toString() {
        return "IPCDevice{" +
                "mac='" + mac + '\'' +
                ", rtspAddr='" + rtspAddr +
                '}';
    }
}
