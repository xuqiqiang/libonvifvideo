
#include "localsocket.h"
#include "common.h"

#define SOCKET_ERROR (-1)

int create_local_socket(char *ip, int port)
{
    int socketFd;

	int  option = 1;
	int  bufSize = 12800;
	struct sockaddr_in HostAddress;

	socketFd = socket(AF_INET, SOCK_STREAM, 0);

	DPRINT(VIDEO_LOG_INFO,"ip:%s port:%d\n", ip, port);
	if(socketFd == SOCKET_ERROR )
	{
		DPRINT(VIDEO_LOG_ERR,"create socket error:%s\n",strerror(errno));
		return -1;
	}

	memset(&HostAddress, 0, sizeof(struct sockaddr));
    HostAddress.sin_family = AF_INET;          // host byte order
    HostAddress.sin_port   = htons(port);        // short, network byte order
	HostAddress.sin_addr.s_addr = inet_addr(ip);
    memset(&(HostAddress.sin_zero), 0, 8);
    
	DPRINT(VIDEO_LOG_DEBUG,"start connect\n");
	if(connect(socketFd,(struct sockaddr *)&HostAddress, sizeof(struct sockaddr)) == -1) {
		DPRINT(VIDEO_LOG_ERR,"Failed to connect to cloud server %s:%d (%s). Marking it as bad and will retry if possible\n", ip, port, strerror(errno));
		close(socketFd);
		return -1;
	}


    if ( SOCKET_ERROR == setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR,(char *)&option, sizeof(int)) )
	{  //允许重用本地地址
		DPRINT(VIDEO_LOG_ERR,"Failed setsockopt error:%s\n",  strerror(errno));
		close(socketFd);
		return -1;
	}

	if ( SOCKET_ERROR == setsockopt(socketFd, SOL_SOCKET, SO_RCVBUF, (char *)&bufSize, sizeof(int)) )
	{// set receive buffer size
		DPRINT(VIDEO_LOG_ERR,"Failed setsockopt error:%s\n",  strerror(errno));
		close(socketFd);
		return -1;
	}

	int flag_tcp_nodelay = 1;
	if (SOCKET_ERROR == setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag_tcp_nodelay, sizeof(int)))
	{
		DPRINT(VIDEO_LOG_ERR,"Failed setsockopt error:%s\n",  strerror(errno));
		close(socketFd);
		return -1;
	}	
	//设非阻塞模式
	fcntl(socketFd, F_SETFL, O_NONBLOCK);

	DPRINT(VIDEO_LOG_DEBUG,"socket create success\n");
	return socketFd;
}

static int GetTickCount()
{
	struct timeval  tm;
	gettimeofday(&tm, NULL);
	return tm.tv_sec*1000 + tm.tv_usec/1000;
}

int sendIPCData(int socketFd, char *pBuf, int len, int timeout)
{
    int ret = -1;

	int		Offset=0, ExpireTime=0;
	fd_set	WriteSet;
	struct timeval	tv, *pTimeval;

    
	if (!pBuf || len<=0)
		return -1;
	
	if (timeout==-1)
		pTimeval=NULL;
	else
	{
		pTimeval=&tv;
		ExpireTime=GetTickCount()+timeout;
	}
	ret =0;
	while(1)
	{
		FD_ZERO(&WriteSet);
		FD_SET(socketFd, &WriteSet);
		
		if (pTimeval)
		{
			tv.tv_sec=timeout/1000;
			tv.tv_usec=(timeout%1000)*1000;
		}
		
		ret=select(socketFd+1, NULL, &WriteSet, NULL, pTimeval);
		if (ret<=0)
		{
            DPRINT(VIDEO_LOG_DEBUG,"select=========%d\n",ret);
			break;
		}
		ret=send(socketFd, pBuf+Offset, len-Offset, 0);
		if (ret<=0)
		{
            DPRINT(VIDEO_LOG_DEBUG,"send=========%d\n",ret);
			break;
		}
		
		Offset+=ret;
		
		if (Offset<len)
		{
			if (pTimeval)
			{
				timeout=ExpireTime-GetTickCount();
				if (timeout<=0)
				{
					ret=Offset;
					break;
				}
			}
		}
		else
		{
			ret=Offset;
			break;
		}
	}

    //DPRINT(VIDEO_LOG_DEBUG,"send rtp data len:%d\n", len);
	return ret;
}

