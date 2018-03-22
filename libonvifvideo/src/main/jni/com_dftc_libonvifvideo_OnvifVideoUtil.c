//
// Created by Administrator on 2016/5/16 0016.
//
#include "com_dftc_libonvifvideo_OnvifVideoUtil.h"
#include "rtp2mp4/writemp4lib.h"
#include <string.h>
#include <jni.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <android/log.h>
#include "rtp2mp4/crc32.h"


#define LOG_TAG "===rtp2mp4==="

#define LOGI(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__) // 定义LOGD类型
/*
 * Class:     com_dftc_libonvifvideo_OnvifVideoUtil
 * Method:    initVideo
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_dftc_libonvifvideo_OnvifVideoUtil_initVideo
  (JNIEnv *env, jobject thiz,jstring dir){
  const char *cdir = (*env)->GetStringUTFChars(env, dir, 0);
  unsigned long hand = InitVideo(cdir);
  //LOGI("init-hand %u", hand);
  //LOGI("init-dir %s", cdir);
  return (jlong)hand;
}

/*
 * Class:     com_dftc_libonvifvideo_OnvifVideoUtil
 * Method:    releaseVideo
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_dftc_libonvifvideo_OnvifVideoUtil_releaseVideo
  (JNIEnv *env, jobject thiz, jlong hand){
  unsigned long chand = hand;
  //LOGI("release-hand %u", chand);
  ReleaseVideo(chand);
  //LOGI("release-result %d", 1);
}

/*
 * Class:     com_dftc_libonvifvideo_OnvifVideoUtil
 * Method:    addIPC
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_dftc_libonvifvideo_OnvifVideoUtil_addIPC
  (JNIEnv *env, jobject thiz, jlong hand, jstring mac, jstring rtsp){
  unsigned long chand = hand;
  const char *cmac = (*env)->GetStringUTFChars(env, mac, 0);
  const char *crtsp = (*env)->GetStringUTFChars(env, rtsp, 0);
  //LOGI("add-hand %u", chand);
  //LOGI("add-mac %s", cmac);
  //LOGI("add-rtsp %s", crtsp);
  int result = AddIPC(chand,cmac,crtsp,0);
  //LOGI("add-result %d", result);
  return (jint)result;
}

/*
 * Class:     com_dftc_libonvifvideo_OnvifVideoUtil
 * Method:    deleteIPC
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_dftc_libonvifvideo_OnvifVideoUtil_deleteIPC
  (JNIEnv *env, jobject thiz, jlong hand, jstring mac){
  unsigned long chand = hand;
  const char *cmac = (*env)->GetStringUTFChars(env, mac, 0);
  //LOGI("release-hand %u", chand);
  //LOGI("release-mac %s", cmac);
  int result = DeleteIPC(chand,cmac);
  //LOGI("release-result %d", result);
  return (jint)result;
}

/*
 * Class:     com_dftc_libonvifvideo_OnvifVideoUtil
 * Method:    sendIPCData
 * Signature: (JLjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_dftc_libonvifvideo_OnvifVideoUtil_sendIPCData
  (JNIEnv *env, jobject thiz, jlong hand, jstring mac, jint port){
  unsigned long chand = hand;
  const char *cmac = (*env)->GetStringUTFChars(env, mac, 0);
  int cport = port;
  //LOGI("send-hand %u", chand);
  //LOGI("send-mac %s", cmac);
  //LOGI("send-port %d", cport);
  int result = SendIPCData(chand,cmac,cport);
  //LOGI("send-result %d", result);
  return (jint)result;
}
/*
 * Class:     com_dftc_libonvifvideo_OnvifVideoUtil
 * Method:    stopIPCData
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_dftc_libonvifvideo_OnvifVideoUtil_stopIPCData
  (JNIEnv *env, jobject thiz, jlong hand, jstring mac){
  unsigned long chand = hand;
  const char *cmac = (*env)->GetStringUTFChars(env, mac, 0);
  //LOGI("stop-hand %ld", chand);
  //LOGI("stop-mac %s", cmac);
  int result = StopIPCData(chand,cmac);
  //LOGI("stop-result %d", result);
  return (jint)result;
}

/*
 * Class:     com_dftc_libonvifvideo_OnvifVideoUtil
 * Method:    setVideoInfo
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_com_dftc_libonvifvideo_OnvifVideoUtil_setVideoInfo
  (JNIEnv *env, jobject thiz,jlong hand, jint time, jint day, jint size){
  int ctime = time;
  int cday = day;
  int csize = size;
  long chand = hand;
  //LOGI("setVideo-time %d", ctime);
  //LOGI("setVideo-hand %u", chand);
  //LOGI("setVideo-day %d", cday);
  //LOGI("setVideo-size %d", csize);
  SetVideoInfo(chand,ctime,cday,csize);
  //LOGI("setVideo-result %d", 1);
}

JNIEXPORT jlong JNICALL Java_com_dftc_libonvifvideo_OnvifVideoUtil_crc32
        (JNIEnv *env, jobject thiz,jstring fileName){
    const char *file = (*env)->GetStringUTFChars(env, fileName, 0);
    unsigned int crc32Value = 0xFFFFFFFF;
    int result = compare_crc(file,&crc32Value);
    //LOGI("crc32 result -> %d",result);
    //LOGI("crc32 value - > %u",crc32Value);
  return (jlong)crc32Value;
}

