#ifndef LOCAL_SOCKET
#define LOCAL_SOCKET

#ifdef __cplusplus 
extern "C" { 
#endif

#include "dlist.h"
#include "common.h"

#ifndef MUTEX
#define MUTEX pthread_mutex_t
#endif
typedef struct _SEND_RTP_DATA_
{
    DList *rtpList;
    MUTEX rtpmutex;
    int send_sock;
    int send_flag;
    int spsflag;
    pthread_cond_t  sCond;
    pthread_t sThreadId;
    int send_init;
    int send_port;
    int m_exit;
}SENDRTPDATA;

int create_local_socket(char *ip, int port);
int sendIPCData(int socketFd, char *pBuf, int len, int timeout);

#ifdef __cplusplus
}
#endif

#endif
