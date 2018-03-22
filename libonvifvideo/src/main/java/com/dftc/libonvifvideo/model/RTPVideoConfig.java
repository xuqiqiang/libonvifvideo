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
public class RTPVideoConfig {
    protected int time; //分钟
    protected int day; //分钟
    protected int size; //M
    protected RTPConnection connection;

    public RTPVideoConfig(RTPConnection connection, int time, int day, int size) {
        this.connection = connection;
        this.time = time;
        this.day = day;
        this.size = size;
    }

    public RTPVideoConfig(RTPConnection connection) {
        this.connection = connection;
    }


    public RTPConnection getConnection() {
        return connection;
    }

    public RTPVideoConfig configTime(int time) {
        this.time = time;
        return this;
    }

    public RTPVideoConfig configDay(int day) {
        this.day = day;
        return this;
    }

    public RTPVideoConfig configSize(int size) {
        this.size = size;
        return this;
    }

    public int getTime() {
        return time;
    }

    public int getDay() {
        return day;
    }

    public int getSize() {
        return size;
    }
}
