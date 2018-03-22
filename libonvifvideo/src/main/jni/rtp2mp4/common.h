
#ifndef RECORD_COMMON_HEADER
#define RECORD_COMMON_HEADER

//IO
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h> 
#include <math.h>
#include <unistd.h>  
#include <fcntl.h> 
//THREAD
#include <pthread.h>

//NETWORK
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h> 
#include <signal.h>
#include <sys/types.h>
#include <sys/un.h> 
#include <linux/tcp.h>

#define VIDEO_LOG_DEBUG 0
#define VIDEO_LOG_INFO 1
#define VIDEO_LOG_ERR 2



//extern int logLevel;

#ifdef CONFIG_VIDEO_LOG
#define VIDEO_LOG_LEVEL 0
#else
#define VIDEO_LOG_LEVEL 2
#endif

#ifdef DEBUG_ANDROID
#include <android/log.h>
#define LOG_TAG "===wangdu==="
#define LOGI(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__) // 定义LOGD类型

#define DPRINT(level,format, args...) do {if(level>=VIDEO_LOG_LEVEL){\
                                                LOGI("DP[%s-%s:%d] "format, __FUNCTION__, __FILE__, __LINE__, ##args);}} while (0)    
#else
#define DPRINT(level,format, args...) do {if(level>=VIDEO_LOG_LEVEL){\
                                                printf("DP[%s-%s:%d] "format, __FUNCTION__, __FILE__, __LINE__, ##args);}} while (0)    
#endif


#define VIDEO_ERR                        -1
#define VIDEO_NEW_ERROR                  0
#define VIDEO_SUCCESS                  0
#define VIDEO_FATAL_ERROR                -1

#endif
