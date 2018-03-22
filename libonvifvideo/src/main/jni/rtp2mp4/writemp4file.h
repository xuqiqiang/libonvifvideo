#ifndef _WRITEMP4_H_
#define _WRITEMP4_H_ 

#ifdef __cplusplus
extern "C" 
{
#endif
#include "dlist.h"
#include "common.h"

#ifndef MUTEX
#define MUTEX pthread_mutex_t
#endif

typedef struct _WriteOperation_
{
    DList *m_dlist; //文件
    int m_index;
}WRITEOPT;

typedef struct _frame_t
{
    uint32_t i_frame_size;
    uint8_t* p_frame;
    struct _frame_t* p_next;
} frame_t;

typedef struct _WriteInfo_
{
    int m_time; //每隔5分钟写一次文件
    int m_day; //写多少天
    int m_size; //写文件大小

    
    int i_buf_num;
    frame_t* p_frame_buf;
    frame_t* p_frame_header;
    MUTEX mutex;
    pthread_cond_t  cond;
    pthread_t threadId;

    #ifdef WRITE_FILE_TEST
    int fileCount;
    #else
    WRITEOPT writeOpt;
    #endif   

    int width;
    int height;
    char mac[32];
    char sps[32];
    int sps_len;
    char pps[32];
    int pps_len;

    int m_exit;
    int m_flag;
    void *fileHand;
}WRITEINFO;

int writemp4file(WRITEINFO* p_write);
int get_file(char *mac, DList *dlist, int file_count);
void set_write_path(char *path);
int removeRcFile(char *mac);

#ifdef __cplusplus
}
#endif

#endif