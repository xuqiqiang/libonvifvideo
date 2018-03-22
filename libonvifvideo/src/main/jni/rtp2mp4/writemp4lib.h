#ifndef _WRITEMP4_H_
#define _WRITEMP4_H_ 

#ifdef __cplusplus
extern "C" 
{
#endif

#define MP4_FILE 0
#define H264_FILE 1

unsigned long InitVideo(char *path);
void ReleaseVideo(unsigned long hand);
int AddIPC(unsigned long hand, char *mac, char *rtsp, int flag);
int DeleteIPC(unsigned long hand, char *mac);
int SendIPCData(unsigned long hand, char *mac, int port);
int StopIPCData(unsigned long hand, char *mac);
void SetVideoInfo(unsigned long hand, int time, int day, int size);
void SetLogLevel(int mLevel);

#ifdef __cplusplus
}
#endif

#endif

