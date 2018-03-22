#include "writemp4lib.h"

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>  
#include <pthread.h>   
#include <semaphore.h>  
#include <sys/ioctl.h>  
#include <net/if.h>  
#include <signal.h>

int get_mac(char* mac)
{
    int sockfd;
    struct ifreq tmp;   
    char mac_addr[30];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0)
    {
        printf("create socket fail,error:%s\n",strerror(errno));
        return -1;
    }

    memset(&tmp,0,sizeof(struct ifreq));
    strncpy(tmp.ifr_name,"eth0",sizeof(tmp.ifr_name)-1);
    if( (ioctl(sockfd,SIOCGIFHWADDR,&tmp)) < 0 )
    {
        printf("mac ioctl error:%s\n",strerror(errno));
        return -1;
    }

    sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)tmp.ifr_hwaddr.sa_data[0],
            (unsigned char)tmp.ifr_hwaddr.sa_data[1],
            (unsigned char)tmp.ifr_hwaddr.sa_data[2],
            (unsigned char)tmp.ifr_hwaddr.sa_data[3],
            (unsigned char)tmp.ifr_hwaddr.sa_data[4],
            (unsigned char)tmp.ifr_hwaddr.sa_data[5]
            );
    printf("local mac:%s\n", mac_addr);
    close(sockfd);
    memcpy(mac,mac_addr,strlen(mac_addr));

    return 0;
}


#define qq(s, i, d)  s##i##d

int main(int argc, char** argv)
{
    long hret = 0;
    //char *url = "rtsp://192.168.100.69:554/stream2";
    //char *url = "rtsp://192.168.100.74:8555/H264SubStream";
    //char *url = "rtsp://192.168.100.74:8555/H264SubStream";
    char *url ="rtsp://192.168.0.87:554/stream2";
    //char *url ="rtsp://192.168.100.73:8555/H264SubStream";
    //char *url = "rtsp://192.168.100.75:554/stander/livestream/0/1";
    //char *url ="rtsp://192.168.100.63:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_2";
    //char *url ="rtsp://192.168.100.71:554/cam/realmonitor?channel=1&subtype=1&unicast=true&proto=Onvif";
    //int port = 3000;
    char *mac="11-22-33-44-55";
    char ch; 
    char *url1="rtsp://192.168.0.70:554/stander/livestream/0/1";
    char *mac1="11-22-33-44-55-66";

    
    printf("url:%s\n",url);
    printf("input your strings: p (start write mp4)  s(stop write) i(init video) r(release video) e(exit) l(log) h(help)\n");  


    struct timeval start, end;
    while(1){  
        ch = getchar();
        gettimeofday(&start, NULL);
        if(ch == 'p')
        {
            AddIPC(hret, mac, url, 0);
            gettimeofday(&end, NULL);
            printf("start write mp4\n");
            printf("interval = %f\n", (1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec))/1000.0);
        }
        else if(ch == '5')
        {
            
            AddIPC(hret, mac1, url1, 0);
            printf("start write mp4:%s\n",url1);
        }
        else if(ch == '6')
        {
            
            DeleteIPC(hret, mac1);
            printf("stop write mp4:mac:%s\n",mac1);
        }
        else if(ch == 's')
        {
            
            DeleteIPC(hret, mac);
            gettimeofday(&end, NULL);
            printf("interval = %f\n", (1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec))/1000.0);
            printf("stop write mp4\n");
        }
        else if(ch == 'i')
        {
            
            char *tmpPath = "/home/walter/share";
            hret = InitVideo(tmpPath);
            gettimeofday(&end, NULL);
            printf("interval = %f\n", (1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec))/1000.0);
            printf("init video\n");
            if(hret <= 0)
            {
                printf("init video error\n");
                return -1;
            }
        }
        else if(ch == 'r')
        {
            
            ReleaseVideo(hret);
            gettimeofday(&end, NULL);
            printf("interval = %f\n", (1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec))/1000.0);
            printf("relead video\n");
        }
         else if(ch == '7')
        {
            
            SendIPCData(hret, mac, 8000);
            printf("start send video\n");
        }
        else if(ch == '8')
        {
            
            StopIPCData(hret, mac);
            printf("stop send video\n");
        }
        else if(ch == 'e')
        {
            printf("exit\n");
            return 0;
        }
        else if(ch == 'l')
        {
            SetLogLevel(2);
            printf("set log level\n");
        }
        else if(ch == 'h')
        {
            printf("input your strings: p (start write mp4)  s(stop write) i(init video) r(release video) e(exit) l(log) h(help)\n"); 
        }
        else
        {
            printf("not cmd\n");
        }
    }  
    
    return 0;
}

