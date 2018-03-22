#include "common.h"
#include "writemp4lib.h"
#include "dlist.h"
#include "rtp.h"
#include "rtsp.h"
#include "localsocket.h"
#include "writemp4file.h"
#include "log.h"
#include <signal.h>

#define ADDR_LEN 128
#define MAC_LEN 18

#define HAND_COUNT 30

typedef struct _WriteMp4_
{
	unsigned long m_rtp;
    char m_mac[MAC_LEN];
	char m_rtspUrl[ADDR_LEN];
	int m_rtpPort;
    unsigned long m_rtsp;
    int m_flag;
}WRITEMP4;

typedef struct _TIME_INFO_
{
    int m_time; //
    int m_day; //
    int m_size; //
}TIME_INFO;

typedef struct _VIDEO_INFO_
{
    DList *dlist;
    pthread_mutex_t  list_lock;
    pthread_t thread_id;
	int thread_quit_flag;
} VIDEO_INFO;

static TIME_INFO g_time;
static int rtpPort = 18000;

//int logLevel = 0;

WRITEMP4 * CreatNode()
{
    WRITEMP4 *node = (WRITEMP4*) malloc(sizeof(WRITEMP4));
    if (node == NULL)
    {
        return NULL;
    }
    memset(node, 0, sizeof(WRITEMP4));
    return node;
}

void sighand(int signo){  
    DPRINT(VIDEO_LOG_ERR, "Thread %u in signal handler\n", (unsigned int )pthread_self());  
} 
static void handListThread(void* arg)
{    
    //pthread_detach(pthread_self());

    VIDEO_INFO * p_video = (VIDEO_INFO*) arg;
    if (p_video == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"arg is null\n");
        return;
    }

    DPRINT(VIDEO_LOG_INFO,"===================handListThread thread start id:%u==============================\n",pthread_self());
    int count = 0;
    char mac[MAC_LEN];
	char rtspUrl[ADDR_LEN];
    while(p_video->thread_quit_flag)
    {
        //pthread_testcancel();
        if(count < HAND_COUNT)
        {
            count++;
            sleep(1);
            continue;
        }


        count = 0;
        memset(mac, 0, MAC_LEN);
        memset(rtspUrl, 0, ADDR_LEN);
        
        pthread_mutex_lock(&p_video->list_lock);
        DList *dlist = p_video->dlist;   

        DListNode* iter = dlist->first;
        while(iter != NULL)
        {
            WRITEMP4 *tmp = (WRITEMP4 *)iter->data;

            if(get_err_count(tmp->m_rtp) < 0)
            {
                DPRINT(VIDEO_LOG_INFO,"net error delete ipc mac:%s rtsp:%s \n", tmp->m_mac, tmp->m_rtspUrl);
                memcpy(mac, tmp->m_mac, strlen(tmp->m_mac));
                memcpy(rtspUrl, tmp->m_rtspUrl, strlen(tmp->m_rtspUrl));
                StopRecordMp4(tmp);
                StartRecordMp4(tmp);
            }
            else
            {
                rtsp_keepalive(tmp->m_rtsp);
                initSendRtpSocket(tmp->m_rtp);
            }
            iter = iter->next;
        }
        pthread_mutex_unlock(&p_video->list_lock);
        
    }

    DPRINT(VIDEO_LOG_ERR,"===================handListThread thread exit id:%u==============================\n",pthread_self());
}




static void readLogLevelIni(char *path)
{
    char bufFile[256];
    memset(bufFile, 0, 256);
    if(path == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"read ini log level error,path is null");
        return;
    }
    strcat(bufFile, path);
    strcat(bufFile, "/video.ini");
    int tmpLevel = 0;
   // tmpLevel = getIniKeyInt("LOG","level",bufFile);

    DPRINT(VIDEO_LOG_ERR,"read ini log level:%d\n", tmpLevel);
    
    if(tmpLevel>=0 && tmpLevel <= 2)
    {
        //logLevel = tmpLevel;
    }
    
}

unsigned long startRtsp(char *url, int rtpPort)
{
    unsigned long hRtsp = 0;
    int ret =0;
	int len;
	hRtsp = rtsp_open(url, rtpPort, &len, NULL);
    if(hRtsp <= 0)
    {
        DPRINT(VIDEO_LOG_ERR,"rtsp_open error\n");
        return VIDEO_NEW_ERROR;
    }
	ret = rtsp_play(hRtsp,0);
    if(ret < 0)
    {
        DPRINT(VIDEO_LOG_INFO,"rtsp_play error\n");
        rtsp_close(hRtsp);
        return VIDEO_NEW_ERROR;
    }

    return hRtsp;
    
}

int StartRecordMp4(WRITEMP4 *pWrite)
{
    unsigned long hRtsp = 0;
    unsigned long hRtp = 0;

    if( pWrite == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"pwrite is null\n");
        return VIDEO_ERR;
    }


    
    hRtp = rtp_init(NULL, pWrite->m_mac, pWrite->m_rtpPort, pWrite->m_flag);
    if(hRtp <= 0)
    {
        DPRINT(VIDEO_LOG_ERR,"start rtp error\n");
        return VIDEO_ERR;
    }
    pWrite->m_rtp = hRtp;

    //set_writeInfo(hRtp, 5, 60, 8);
    //set_writeInfo(hRtp, 1, 10, 8);
    set_writeInfo(hRtp, g_time.m_time, g_time.m_day, g_time.m_size);

    
    #if 1
    hRtsp = startRtsp(pWrite->m_rtspUrl, pWrite->m_rtpPort);
    if(hRtsp <= 0)
    {
        DPRINT(VIDEO_LOG_INFO,"start rtsp error\n");
        rtp_deinit(hRtp);
        pWrite->m_rtp = 0;
        return VIDEO_ERR;
    }

    pWrite->m_rtsp = hRtsp;
  
    #endif
    return VIDEO_SUCCESS;
}


int StopRecordMp4(WRITEMP4 *pWrite)
{
    if(pWrite == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"\n==============stop record mp4 error====================\n");
        return VIDEO_ERR;
    } 

    DPRINT(VIDEO_LOG_ERR,"\n==============stop record mp4 start====================\n");
    rtsp_close(pWrite->m_rtsp);
    pWrite->m_rtsp = 0;
    rtp_deinit(pWrite->m_rtp);
    pWrite->m_rtp =0;

    return VIDEO_SUCCESS;
}

unsigned long InitVideo(char *path)
{
    signal(SIGPIPE, SIG_IGN);

    sigset_t signal_mask;
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGPIPE);
    int rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
        DPRINT(VIDEO_LOG_ERR,"block sigpipe error");
    }
    //readLogLevelIni(path);
    VIDEO_INFO *hand = NULL;

    hand = (VIDEO_INFO *)malloc(sizeof(VIDEO_INFO));
    if(hand == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"malloc VIDEO_INFO error\n");
        return VIDEO_NEW_ERROR;
    }
    memset(hand, 0, sizeof(VIDEO_INFO));
    
    DList *dlist;
    dlist = dlist_create();
    if(dlist == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"create dlist error\n");
        free(hand);
        return VIDEO_NEW_ERROR;
    }

    hand->dlist = dlist;
    hand->thread_quit_flag = 1;
    pthread_mutex_init(&hand->list_lock, NULL);
    pthread_create(&hand->thread_id, 0, (void*)handListThread, (void *)hand);

    set_write_path(path);
    rtpPort = 18000;

    g_time.m_time = 5;
    g_time.m_day = 7*24*60;
    g_time.m_size =8;

    DPRINT(VIDEO_LOG_ERR,"\n==============init video success====================\n");
    return (unsigned long)hand;
}

void ReleaseVideo(unsigned long hand)
{
    VIDEO_INFO * vhand = NULL;
    vhand = (VIDEO_INFO *)hand;

    if(vhand == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"hand error\n");
        return;
    }

    DPRINT(VIDEO_LOG_ERR,"\n==============release video start====================\n");
    vhand->thread_quit_flag = 0;
    pthread_join(vhand->thread_id,NULL);
    
    pthread_mutex_lock(&vhand->list_lock);
    DList *dlist = vhand->dlist;  
    DListNode* iter = dlist->first;

    DPRINT(VIDEO_LOG_ERR,"\n==============release video start delete dlist count:%d=====\n", dlist_length(dlist));
    while(iter != NULL)
    {
        WRITEMP4 *tmp = (WRITEMP4 *)iter->data;

        DPRINT(VIDEO_LOG_ERR,"delete mac:%s\n", tmp->m_mac);
        
        StopRecordMp4(tmp);
        //sleep(1);
        DPRINT(VIDEO_LOG_ERR,"delete ================node======\n");
        iter = dlist_delete_node_ex(dlist,iter);
        DPRINT(VIDEO_LOG_ERR,"============r===============delete node , count:%d\n", dlist_length(dlist));
        //iter = iter->next;
    }
    dlist_destroy(dlist);
    pthread_mutex_unlock(&vhand->list_lock);
    pthread_mutex_destroy(&(vhand->list_lock));
    
    //sleep(1);
    free(vhand);
    DPRINT(VIDEO_LOG_ERR,"release hand over\n");
}

int AddIPC(unsigned long hand, char *mac, char *rtsp, int flag)
{
    if(mac == NULL || rtsp == NULL || hand <= 0)
    {
        DPRINT(VIDEO_LOG_ERR,"mac or rtsp or hand error\n");
        return VIDEO_ERR;
    }

    VIDEO_INFO * vhand = NULL;
    vhand = (VIDEO_INFO *)hand;

    pthread_mutex_lock(&vhand->list_lock);
    DList *dlist = vhand->dlist;   
    
    
    DPRINT(VIDEO_LOG_ERR,"add mac :%s\n", mac);
    DPRINT(VIDEO_LOG_ERR,"add rtsp :%s\n", rtsp);

    DListNode* iter = dlist->first;    
    while(iter != NULL)
    {
        WRITEMP4 *tmp = (WRITEMP4 *)iter->data;

        if(strncmp(tmp->m_mac, mac, strlen(mac)) == 0)
        {
            DPRINT(VIDEO_LOG_DEBUG,"dlist has mac %s\n", mac);
            pthread_mutex_unlock(&vhand->list_lock);
            return 1;
        }
        iter = iter->next;
    }

    WRITEMP4 *ipcNode = CreatNode();
    memcpy(ipcNode->m_mac, mac, strlen(mac));
    memcpy(ipcNode->m_rtspUrl,rtsp, strlen(rtsp));
    ipcNode->m_flag = flag;
    ipcNode->m_rtpPort = rtpPort;

    if(StartRecordMp4(ipcNode) < 0)
    {
        DPRINT(VIDEO_LOG_ERR,"StartRecordMp4 error\n");
        free(ipcNode);
        pthread_mutex_unlock(&vhand->list_lock);
        return VIDEO_ERR;
    }
    dlist_append(dlist, ipcNode);
    rtpPort = rtpPort+2;
    pthread_mutex_unlock(&vhand->list_lock);
    
    DPRINT(VIDEO_LOG_ERR,"StartRecordMp4 success, rtp port:%d\n", rtpPort);  
    return VIDEO_SUCCESS;
}

int DeleteIPC(unsigned long hand, char *mac)
{
    if(mac == NULL  || hand <= 0)
    {
        DPRINT(VIDEO_LOG_ERR,"mac or rtsp or hand error\n");
        return VIDEO_ERR;
    }
    
    VIDEO_INFO * vhand = NULL;
    vhand = (VIDEO_INFO *)hand;

    pthread_mutex_lock(&vhand->list_lock);
    DList *dlist = vhand->dlist;  

    DPRINT(VIDEO_LOG_ERR,"delete mac :%s\n", mac);
    DListNode* iter = dlist->first;

    while(iter != NULL)
    {
        WRITEMP4 *tmp = (WRITEMP4 *)iter->data;

        DPRINT(VIDEO_LOG_ERR,"delete mac:%s\n", tmp->m_mac);
        if(strncmp(tmp->m_mac, mac, strlen(mac)) == 0)
        {
            DPRINT(VIDEO_LOG_ERR,"delete ================\n");
            StopRecordMp4(tmp);
            //sleep(1);
            iter = dlist_delete_node_ex(dlist,iter);
            DPRINT(VIDEO_LOG_ERR,"===========================delete node , count:%d\n", dlist_length(dlist));
            break;
        }
        iter = iter->next;
    }

    pthread_mutex_unlock(&vhand->list_lock);
    return VIDEO_SUCCESS;
}

int SendIPCData(unsigned long hand, char *mac, int port)
{
    int ret = 0;
    char * ip = "127.0.0.1";
    
    if(mac == NULL || hand <= 0)
    {
        DPRINT(VIDEO_LOG_ERR,"mac  or hand error\n");
        return VIDEO_ERR;
    }
    
    VIDEO_INFO * vhand = NULL;
    vhand = (VIDEO_INFO *)hand;

    pthread_mutex_lock(&vhand->list_lock);
    DList *dlist = vhand->dlist;  

    DPRINT(VIDEO_LOG_ERR,"send mac :%s\n", mac);
    DListNode* iter = dlist->first;

    while(iter != NULL)
    {
        WRITEMP4 *tmp = (WRITEMP4 *)iter->data;

        DPRINT(VIDEO_LOG_ERR,"==============current mac:%s===============\n", tmp->m_mac);
        if(start_send(tmp->m_rtp, mac, port)== 0)
        {
            DPRINT(VIDEO_LOG_ERR,"start play success\n");
            break;
        }
        iter = iter->next;
    }

    pthread_mutex_unlock(&vhand->list_lock);
    if(iter == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"not find ipc\n");
        return VIDEO_ERR;
    }
    return VIDEO_SUCCESS;
}

int StopIPCData(unsigned long hand, char *mac)
{
    
    if(mac == NULL || hand <= 0)
    {
        DPRINT(VIDEO_LOG_ERR,"mac  or hand error\n");
        return VIDEO_ERR;
    }
    
    VIDEO_INFO * vhand = NULL;
    vhand = (VIDEO_INFO *)hand;

    pthread_mutex_lock(&vhand->list_lock);
    DList *dlist = vhand->dlist; 

    DPRINT(VIDEO_LOG_ERR,"stop mac :%s\n", mac);
    DListNode* iter = dlist->first;

    while(iter != NULL)
    {
        WRITEMP4 *tmp = (WRITEMP4 *)iter->data;

        if(stop_send(tmp->m_rtp, mac) == 0)
        {
            DPRINT(VIDEO_LOG_ERR,"stop mac ================\n");
            break;
        }
        iter = iter->next;
    }

    pthread_mutex_unlock(&vhand->list_lock);
    
    if(iter == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"not find ipc\n");
        return VIDEO_ERR;
    }
    return VIDEO_SUCCESS;
}

void SetVideoInfo(unsigned long hand, int time, int day, int size)
{
    //DList *dlist = NULL;   
    //dlist = (DList *)hand;

    if(time <=0 || day <= 10 || size <= 0)
    {
        DPRINT(VIDEO_LOG_ERR,"para error\n");
        return;
    }

    g_time.m_day = day;
    g_time.m_size = size;
    g_time.m_time = time;

    /*
    DListNode* iter = dlist->first;
    
    while(iter != NULL)
    {
        WRITEMP4 *tmp = (WRITEMP4 *)iter->data;

        set_writeInfo(tmp->m_rtp,time, day, size);
        iter = iter->next;
    }
    */
    
}

void SetLogLevel(int mLevel)
{
    //logLevel = mLevel;
}
