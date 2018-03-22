#include "common.h"
#include "mp4v2/mp4v2.h"
#include "writemp4file.h"
#include "dlist.h"
#include "crc32.h"
#include "log.h"
#include <sys/statfs.h>
#include <sys/vfs.h>


//#define NOT_WRITE_FILE

#define FILE_NAME_LENGTH 256

#define TIME_PER 1

#include <dirent.h>

#define MAX 1024

char g_write_path[64];

void set_write_path(char *path)
{
    memset(g_write_path, 0, 64);
    if(path != NULL)
    {
        memcpy(g_write_path, path, strlen(path));
    }
}

int get_file_count(char *dirPath)
{
    DIR *dir;
    struct dirent * ptr;
    int total = 0;
    char path[MAX];
    dir = opendir(dirPath); /* 打开目录*/
    if(dir == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"fail to open dir");
        return -1;
    }

    while((ptr = readdir(dir)) != NULL)
    {
        //顺序读取每一个目录项；
        //跳过“..”和“.”两个目录
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
        {  
            continue;  
        } 
        
        if(ptr->d_type == DT_REG)
        {
            total++;
            DPRINT(VIDEO_LOG_DEBUG,"%s%s\n",dirPath,ptr->d_name);
        }
    }

    closedir(dir);
    return total;
}

void strrpl(char* pDstOut, char* pSrcIn, const char* pSrcRpl, const char* pDstRpl)
{ 
    char* pi = pSrcIn; 
    char* po = pDstOut; 

    int nSrcRplLen = strlen(pSrcRpl); 
    int nDstRplLen = strlen(pDstRpl); 

    char *p = NULL; 
    int nLen = 0; 

    do 
    {
        // 找到下一个替换点
        p = strstr(pi, pSrcRpl); 

        if(p != NULL) 
        { 
            // 拷贝上一个替换点和下一个替换点中间的字符串
            nLen = p - pi; 
            memcpy(po, pi, nLen);

            // 拷贝需要替换的字符串
            memcpy(po + nLen, pDstRpl, nDstRplLen); 
        } 
        else 
        { 
            strcpy(po, pi); 

            // 如果没有需要拷贝的字符串,说明循环应该结束
            break;
        } 

        pi = p + nSrcRplLen; 
        po = po + nLen + nDstRplLen; 

    } while (p != NULL); 
}

int writedir(char *mac)
{
    DIR *dirptr = NULL;
    if((dirptr = opendir(mac)) == NULL)
    {
        int status;
        //status = mkdir(mac, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        status = mkdir(mac, 0777);
        return status;
    }
    
    closedir(dirptr); 

    return 0;
}

int timeout(time_t * startTime,time_t * endTime)
{
    time_t start, end, current;
    current = time(NULL);
    
    if((current >= *startTime) && (current <= *endTime))
    {
       return 0;
    }

    return 1;
}

void getmp4file(char *retFile)
{
    time_t now;
    struct tm *tm_now;
    
    time(&now);
    tm_now = localtime(&now);
    sprintf(retFile,"%d-%d-%d-%d-%d-%d.mp4", tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

}

void getmp4filepath(char *dir, char *retfilepath)
{
    char mp4file[32];
    memset(mp4file, 0, 32);

    writedir(dir);
    getmp4file(mp4file);
    sprintf(retfilepath, "%s/%s", dir, mp4file);
}

void getmp4dir(char *mac, char *retdir)
{
    char mp4dir[32];
    memset(mp4dir, 0, 32);
    char *src=":";
    char *dst="-";
    strrpl(mp4dir,mac,src,dst);
    sprintf(retdir, "%s/%s", g_write_path, mp4dir);
}

static int free_frame(frame_t** pp_frame)
{
    if ((*pp_frame) != NULL)
    {
        free((*pp_frame));
        (*pp_frame) = NULL;
    }
    return 0;
}

int getFirstFileName(char *dir, char *outName)
{
    FILE *fp;
    char tmp[256];
    memset(tmp, 0, 256);
    strcat(tmp, "ls -rt ");
    strcat(tmp, dir);
    strcat(tmp, " | head -n 1 | awk '{print $1}'");
    //DPRINT(VIDEO_LOG_DEBUG,"popen cmd:%s\n",tmp);
	fp = popen(tmp,"r");
    if(fp == NULL)
    {
	    DPRINT(VIDEO_LOG_ERR,"popen error\n");
        return -1;
    }
    fgets(outName, 256, fp);
    pclose(fp);
    return 0;
}

int create_null_file(char *pName, long size)
{
    int  write_size = 0;    
    char buffer[1024];
    memset(buffer, 0x00, sizeof(buffer));
    DPRINT(VIDEO_LOG_DEBUG,"creat null file:%s, size:%ld\n", pName, size);
    FILE *fp = fopen(pName, "wb+");
    if(fp == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"fopen :%s error\n", pName);
        return -1;
    }
        while(size)
       {
        //DPRINT("write size:%ld\n", size);
            if(size < 1024){
                write_size = size;   
            }
            else
                write_size = 1024;

            size -= fwrite(buffer, 1, write_size, fp);
        }
    fclose(fp);  

    return 0;
}

int rename_mp4file(WRITEOPT *writeopt, unsigned int duration, char *name)
{
    return 0;
}

int device_size(char *path)
{
    struct statfs disk_info;
    int ret = 0;
    if(ret == statfs(path, &disk_info) == -1)
    {
		DPRINT(VIDEO_LOG_ERR,"Failed to get file disk infomation,errno:%u, reason:%s\n", errno,strerror(errno));
		return -1;
    }

    DPRINT(VIDEO_LOG_DEBUG,"path :%s free size:%luM\n", path, (disk_info.f_bfree * disk_info.f_bsize) >> 20);
    if(((disk_info.f_bfree * disk_info.f_bsize) >> 20) < 1000)
    {
        return -1;
    }

    return 0;
}

void getmp4filetmp(char *mac, char *retfile, int flag)
{
    char dirpath[FILE_NAME_LENGTH];
    memset(dirpath, 0, FILE_NAME_LENGTH);

    getmp4dir(mac, dirpath);
    writedir(dirpath);
    time_t now;    
    time(&now);
    if(flag)
    {
        sprintf(retfile, "%s/%u-rc.h264", dirpath,(unsigned int)now);
    }
    else
    {
        sprintf(retfile, "%s/%u-rc.mp4", dirpath,(unsigned int)now);
    }
    
}

void getmp4filenew(char *mac, char *retfile)
{
    char dirpath[FILE_NAME_LENGTH];
    memset(dirpath, 0, FILE_NAME_LENGTH);

    getmp4dir(mac, dirpath);
    time_t now;
    struct tm *tm_now;
    
    time(&now);
    tm_now = localtime(&now);
    sprintf(retfile,"%s/%d-%02d-%02d-%02d-%02d-%02d", dirpath,tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

}

void cleanUp(void *arg)
{
    WRITEINFO* pWrite = (WRITEINFO*)arg;
    DPRINT(VIDEO_LOG_ERR,"write file clean up!\n");
    MP4Close(pWrite->fileHand,0);
    pthread_mutex_unlock(&pWrite->mutex);    
}

int writemp4file(WRITEINFO* p_write)
{
    char *mac;
    mac = p_write->mac;
    int ret = 0;
    char filename[FILE_NAME_LENGTH];
    memset(filename, 0, FILE_NAME_LENGTH);

    char filetmp[FILE_NAME_LENGTH];
    memset(filetmp, 0, FILE_NAME_LENGTH);

    char dirpath[FILE_NAME_LENGTH];
    getmp4dir(mac, dirpath);
    
    getmp4filetmp(mac, filename, p_write->m_flag);
    getmp4filenew(mac, filetmp);

    time_t start, end,current;
    current = time(NULL);
    struct tm *t = localtime(&current);
    start = mktime(t);
    end = start + p_write->m_time * 60;

    if (p_write == NULL)
    {
        DPRINT(VIDEO_LOG_ERR," rtp is null ERROR!\n");
        return -1;
    }

    #ifdef WRITE_FILE_TEST
    char tmpOldData[256];
    memset(tmpOldData, 0, 256);
    if(p_write->fileCount  >= (p_write->m_day / p_write->m_time) || (device_size(g_write_path) < 0))
    {
        if(getFirstFileName(dirpath, tmpOldData) == 0)
        {
            DPRINT(VIDEO_LOG_INFO,"dir:%s first file name:%s\n", dirpath, tmpOldData);
            rename(tmpOldData, filename);
            p_write->fileCount--;
        }  
    }
    #else
    
    char *tmpOldData = NULL;
    if(dlist_length(p_write->writeOpt.m_dlist) >= (p_write->m_day / p_write->m_time) || (device_size(dirpath) < 0))
    {
        DPRINT(VIDEO_LOG_DEBUG,"start dlist count :%d ,index:%d\n", dlist_length(p_write->writeOpt.m_dlist),p_write->writeOpt.m_index);
        dlist_get_by_index(p_write->writeOpt.m_dlist, p_write->writeOpt.m_index, (void *)&tmpOldData);
        if(tmpOldData == NULL)
        {
            DPRINT(VIDEO_LOG_INFO,"dlist get file error\n");
            return -1;
        }
        rename(tmpOldData, filename);
    }
    #endif

    #ifdef NOT_WRITE_FILE
    #else
    MP4FileHandle  file;
    MP4TrackId video;
    FILE * fileMp4;

    pthread_mutex_lock(&p_write->mutex);
    
    if(p_write->sps[4] ==0x00 || p_write->pps[4] == 0x00)
    {
        pthread_mutex_unlock(&p_write->mutex);
        DPRINT(VIDEO_LOG_INFO,"not sps and pps, do not write file:%x  %x\n", p_write->sps[4], p_write->pps[4]);
        return -1;
    }

    if(p_write->m_flag == 0)
    {
    char *p[4];  
    p[0] = "isom";  
    p[1] = "iso2";  
    p[2] = "avc1";  
    p[3] = "mp41";  

    file = MP4CreateEx(filename, 0, 1, 1, "isom", 0x00020000, p, 4);  

    if (file == MP4_INVALID_FILE_HANDLE)
    {
        pthread_mutex_unlock(&p_write->mutex);
        DPRINT(VIDEO_LOG_ERR,"open file :%s fialed.\n", filename);
        return -1;
    }
    #if 0
    printf("\nlen:%d\n ",p_write->sps_len);
    int tmptt =0;
    for(tmptt;tmptt<p_write->sps_len;tmptt++)
    {
        printf(" %x",p_write->sps[tmptt]);
    }
    printf("\n\n");
    #endif

    MP4SetTimeScale(file, 90000);
    DPRINT(VIDEO_LOG_INFO,"========sps: width:%d herght:%d sps1:%x sps2:%x sps3:%x\n", p_write->width, p_write->height,p_write->sps[5],p_write->sps[6],p_write->sps[7]);
    video = MP4AddH264VideoTrack(file, 90000, 90000 / 25, p_write->width, p_write->height,
                                            p_write->sps[5], //sps[1] AVCProfileIndication
                                            p_write->sps[6], //sps[2] profile_compat
                                            p_write->sps[7], //sps[3] AVCLevelIndication
                                            3); // 4 bytes length before each NAL unit
    if (video == MP4_INVALID_TRACK_ID)
    {
        pthread_mutex_unlock(&p_write->mutex);
        DPRINT(VIDEO_LOG_ERR,"add video track fialed.\n");
        MP4Close(file,0);
        return -1;
    }
    MP4SetVideoProfileLevel(file, 0x7F);

    MP4AddH264SequenceParameterSet(file,video,p_write->sps+4,p_write->sps_len-4); 
    MP4AddH264PictureParameterSet(file,video,p_write->pps+4,p_write->pps_len-4); 
    }
    else
    {
        fileMp4 = fopen(filename,"ab");
        if(file == NULL)
        {
            pthread_mutex_unlock(&p_write->mutex);
            DPRINT(VIDEO_LOG_ERR,"open filename :%s error\n", filename);
            return -1;
        }
    }
    pthread_mutex_unlock(&p_write->mutex);

    #endif
    unsigned long tmpFileSize = 0;
    p_write->fileHand = file; 
    //pthread_cleanup_push(cleanUp,p_write);
    while(p_write->m_exit)
    { 
        //pthread_testcancel();
        //超时比较
        if(timeout(&start, &end) == 1)
        {
            DPRINT(VIDEO_LOG_DEBUG,"time end write file end\n");
            break;
        }

        //文件大小比较
        if(tmpFileSize >= (p_write->m_size * 1024 * 1024 -1024))
        {
            DPRINT(VIDEO_LOG_DEBUG,"exit file size\n");
            break;
        }

        //写文件
        frame_t* pf = NULL; //frame
        
        pthread_mutex_lock(&p_write->mutex);
        if(p_write->i_buf_num <= 0)
        {
            pthread_cond_wait(&p_write->cond,&p_write->mutex);
        }
        //DPRINT(VIDEO_LOG_DEBUG,"==========wirte rtp mac:%s frame buf  count:%d======\n",p_write->mac,p_write->i_buf_num);
        
        pf = p_write->p_frame_header;
        if (pf != NULL)
        {
            if(pf->i_frame_size >= 4)
            {
                uint32_t* p = (uint32_t*)(&pf->p_frame[0]);
                *p = htonl(pf->i_frame_size -4);
            }

            #ifdef NOT_WRITE_FILE
            #else
                
                //MP4WriteSample(file, video, pf->p_frame, pf->i_frame_size, MP4_INVALID_DURATION, 0, 1);
            if(p_write->m_flag)
            {
                fwrite(pf->p_frame,1,pf->i_frame_size,fileMp4);
            }
            else
            {
                MP4WriteSample(file, video, pf->p_frame, pf->i_frame_size, 90000 / 25, 0, 1);
            }
            #endif

            tmpFileSize += pf->i_frame_size;
            //clear frame.
            p_write->i_buf_num--;
            p_write->p_frame_header = pf->p_next;
            if (p_write->i_buf_num <= 0)
            {
               p_write->p_frame_buf = p_write->p_frame_header;
            }
            free_frame(&pf);
            pf = NULL;

        }
        else
        {
            DPRINT(VIDEO_LOG_DEBUG,"write file buff empty, p_rtp->i_buf_num:%d\n", p_write->i_buf_num);
            pthread_mutex_unlock(&p_write->mutex);
            continue;
            
        }
        pthread_mutex_unlock(&p_write->mutex);
        //DPRINT(VIDEO_LOG_DEBUG,"write  buff  p_rtp->i_buf_num:%d\n", p_write->i_buf_num);

    } 

    //pthread_cleanup_pop(0);

    #ifdef NOT_WRITE_FILE
    #else
    if(p_write->m_flag)
    {
        fclose(fileMp4); 
    }
    else
    {
        MP4Close(file,0); 
    }
    

    unsigned int crc =0xffffffff;
    if(compare_crc(filename, &crc)<0)
    {
        DPRINT(VIDEO_LOG_INFO,"crc error\n");
        return -1;
    }
    unsigned int duration;
    time_t currentTmp;
    currentTmp = time(NULL);
    duration = currentTmp - start;
    char filenameTmp[FILE_NAME_LENGTH];
    memset(filenameTmp, 0, FILE_NAME_LENGTH);
    if(p_write->m_flag)
    {
        sprintf(filenameTmp, "%s-%d-%u.h264", filetmp,duration,crc);
    }
    else
    {
        sprintf(filenameTmp, "%s-%d-%u.mp4", filetmp,duration,crc);
    }
    #endif
    

    #ifdef WRITE_FILE_TEST
    #ifdef NOT_WRITE_FILE
    #else
    rename(filename, filenameTmp);
    #endif
    p_write->fileCount++;
    #else
    if(tmpOldData != NULL)
    {
        rename(filename, filenameTmp);
        memset(tmpOldData, 0, FILE_NAME_LENGTH);
        memcpy(tmpOldData, filenameTmp, strlen(filenameTmp));
        p_write->writeOpt.m_index++;
        if(p_write->writeOpt.m_index >= dlist_length(p_write->writeOpt.m_dlist))
        {
            p_write->writeOpt.m_index = 0;
        }
    }
    else
    {
        char *tmpName;
        tmpName = (char *) malloc(FILE_NAME_LENGTH);
        memset(tmpName, 0, FILE_NAME_LENGTH);
        memcpy(tmpName, filenameTmp, strlen(filenameTmp));
        rename(filename, filenameTmp);
        dlist_append(p_write->writeOpt.m_dlist, tmpName);
    }   
    #endif
    
    return 0;
}

int removeRcFile(char *mac)
{
    char pName[256];
    int ret = 0;

    memset(pName, 0, 256);
    getmp4dir(mac, pName);
    DPRINT(VIDEO_LOG_DEBUG,"======================get file path:%s======================\n", pName);

    FILE *fp;
    char tmp[500];
    memset(tmp, 0, 500);
    strcat(tmp, "rm -rf  ");
    strcat(tmp, pName);
    strcat(tmp, "/*rc*");
    //DPRINT(VIDEO_LOG_DEBUG,"popen cmd:%s\n",tmp);
	fp = popen(tmp,"r");
    if(fp == NULL)
    {
	    DPRINT(VIDEO_LOG_ERR,"popen error\n");
        return -1;
    }
    pclose(fp);
    return 0;
}


//#define  CHECK_DIR_SIZE (1000)
#define  FN_SIZE 256
typedef struct my_struct
{
    char f_name[FN_SIZE];
    time_t ctime;
}f_struct,*p_f_struct;


//将目录下所有文件名放入队列
int neo_check_dir(char *dir, p_f_struct neo_p_head, int filecount) 
{
    DIR * dp;
    struct dirent *dent;
    struct stat st;
    char fn[1024];
    p_f_struct p;

    if(neo_p_head == NULL)
    {
        DPRINT(VIDEO_LOG_DEBUG,"neo_p_head is NULL,need neo_init_check_size()!\n");
        return -1;
    }

    dp = opendir(dir);
    if (!dp) {
        DPRINT(VIDEO_LOG_ERR,"open error[%s]\n", dir);
        return -1;
    }
    p =neo_p_head;
    
    while ((dent = readdir(dp)) != NULL) {
        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
        {  
            continue;  
        }  
        sprintf(fn, "%s/%s", dir, dent->d_name);
        if (stat(fn, &st) == 0) {
            if (S_ISREG(st.st_mode)) {
                snprintf(p->f_name,FN_SIZE,"%s",fn);
                p->ctime = st.st_ctime;
                p++;
                if(p - neo_p_head >= filecount)
                {
                    DPRINT(VIDEO_LOG_DEBUG,"file is mall, max is :%d\n", filecount);
                    break;
                }
            } 
        } else {
            DPRINT(VIDEO_LOG_DEBUG,"can't stat %s\n", fn);
        }
    }
    closedir(dp);
    return 0;
}

//获取缓冲队列尾地址
p_f_struct neo_get_p_end(p_f_struct pstHead)
{
    p_f_struct pstEnd;
    if(pstHead->f_name[0] == 0)
        return NULL;
    pstEnd = pstHead;
    while((pstEnd + 1)->f_name[0] != 0)
        pstEnd ++;
    return pstEnd;
}

//更具key将数组分为两部分
p_f_struct partion(p_f_struct pstHead, p_f_struct pstEnd)
{
    f_struct temp_struct;
    memcpy(&temp_struct, pstHead, sizeof(f_struct));
    while(pstHead != pstEnd)
    {
        while((pstHead < pstEnd) && (pstEnd->ctime >= temp_struct.ctime))
            pstEnd --;
        if(pstHead < pstEnd){
            memcpy(pstHead, pstEnd, sizeof(f_struct));
            pstHead ++;
        }
        while((pstHead < pstEnd) && (pstHead->ctime <= temp_struct.ctime))
            pstHead ++;
        if(pstHead < pstEnd){
            memcpy(pstEnd, pstHead, sizeof(f_struct));
            pstEnd --;
        }
    }
    memcpy(pstHead, &temp_struct, sizeof(f_struct));
    return pstHead;
}
//对扫描到的文件按最后一次修改时间进行排序
void quick_sort(p_f_struct pstHead, p_f_struct pstEnd)
{
    if(pstHead < pstEnd)
    {
        p_f_struct temp_Head = pstHead;
        p_f_struct temp_End = pstEnd;
        p_f_struct pstTemp = partion(temp_Head, temp_End);
        quick_sort(pstHead, pstTemp - 1);
        quick_sort(pstTemp + 1, pstEnd);
    }
}

int get_file(char *mac, DList *dlist, int file_count)
{
    char pName[FILE_NAME_LENGTH];
    int ret = 0;
    p_f_struct neo_p_head = NULL;
    p_f_struct neo_p_end = NULL;

    getmp4dir(mac, pName);

    DPRINT(VIDEO_LOG_DEBUG,"======================get file path:%s======================\n", pName);
    int filecount = 0;
    filecount = get_file_count(pName);
    if(filecount <= 0)
    {
        DPRINT(VIDEO_LOG_INFO,"dir :%s is not file\n", pName);
        return -1;
    }
    
    neo_p_head = (p_f_struct)malloc(sizeof(f_struct) * filecount);
    if(neo_p_head == NULL)
    {
        DPRINT(VIDEO_LOG_ERR,"malloc p_f_struct error\n");
        return -1;
    }
    memset(neo_p_head,0,sizeof(f_struct) * filecount);

    ret = neo_check_dir(pName, neo_p_head, filecount);
    if(ret < 0)
    {
        DPRINT(VIDEO_LOG_INFO,"=======================get file error=======================\n");
        free(neo_p_head);
        return -1;
    }

    neo_p_end = neo_p_head+filecount-1;
    quick_sort(neo_p_head, neo_p_end);
    p_f_struct pstTmp;
    pstTmp = neo_p_head;

    while(filecount)
    {
        if(pstTmp->f_name[0] == 0)
        {
            break;
        }

        if(filecount <= file_count)
        {
            if(strstr(pstTmp->f_name, "rc") == NULL)
            {
                char *tmpName = NULL;
                tmpName = (char *) malloc(FILE_NAME_LENGTH);
                if(tmpName)
                {
                    memset(tmpName, 0 ,FILE_NAME_LENGTH);
                    memcpy(tmpName, pstTmp->f_name, strlen(pstTmp->f_name));
                    dlist_append(dlist, tmpName);  
                    DPRINT(VIDEO_LOG_DEBUG,"=======================append file name:%s=======================\n", tmpName);
                }
            }
            else
            {
                if(remove(pstTmp->f_name) == 0)
                    DPRINT(VIDEO_LOG_DEBUG,"Removed %s\n", pstTmp->f_name);
                else
                    DPRINT(VIDEO_LOG_DEBUG,"remove %s error\n", pstTmp->f_name);
            }
        }
        else
        {
                if(remove(pstTmp->f_name) == 0)
                    DPRINT(VIDEO_LOG_DEBUG,"Removed %s\n", pstTmp->f_name);
                else
                    DPRINT(VIDEO_LOG_DEBUG,"remove %s error\n", pstTmp->f_name);
        }

        filecount--;
        if(filecount <= 0)
        {
            break;
        }
        pstTmp++;
        
    }

    free(neo_p_head);
    DPRINT(VIDEO_LOG_DEBUG,"=======================append file count:%d=======================\n", dlist_length(dlist));
    return 0;
    
}
