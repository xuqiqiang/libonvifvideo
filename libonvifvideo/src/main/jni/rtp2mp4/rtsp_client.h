/*--------------------------------------------------------
	
	COPYRIGHT 2007 (C) DVN (Holdings) Ltd (Hongkong)

	AUTHOR:		wangry@dvnchina.com
	PURPOSE:	kasenna simple rtsp lib
	CREATED:	2007-12-10  

	MODIFICATION HISTORY
	Date        By     Details
	----------  -----  -------

--------------------------------------------------------*/
#ifndef _RTSP_H_
#define _RTSP_H_ 

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "common.h"

//#define CHECK_AND_FREE(a) if ((a) != 0) { free((void *)(a)); (a) = 0;}
#define CHECK_AND_FREE(a) if ((a) != 0) { free((a)); (a) = 0;}
#define RECV_BUFF_DEFAULT_LEN 2048

typedef struct rtsp_client_t {
  char *url;
  char *server_name;
  char session[64];
  int next_cseq;
  int server_socket;
  struct in_addr server_addr;
  int server_port;
  int recv_timeout;
  int send_timeout;
  int m_buffer_len;
  int m_offset_on;
  char m_recv_buffer[RECV_BUFF_DEFAULT_LEN + 1];
}rtsp_client_t;

#ifdef __cplusplus
}
#endif

#endif

