/*--------------------------------------------------------

	COPYRIGHT 2007 (C) DVN (Holdings) Ltd (Hongkong)

	AUTHOR:		wangry@dvnchina.com
	PURPOSE:	kasenna simple rtsp lib
	CREATED:	2007-12-10

	MODIFICATION HISTORY
	Date        By     Details
	----------  -----  -------

--------------------------------------------------------*/
#include <time.h>
#include <semaphore.h>
#include <pthread.h>

#include "rtsp.h"
#include "rtsp_client.h"
#include "rtsp_sock.h"
#include "rtsp_command.h"
#include "rtsp_response.h"


typedef struct _RTSPCLIENT_
{
	rtsp_client_t rtsp_client;
	rtsp_callback_func_t rtsp_callback_func;
	pthread_t thread_id;
	int thread_quit_flag;
    int send_t;
}RTSPCLIENT;
/*-------------------------------------------------------
	commone function
*/

unsigned int get_time()
{
	struct timeval tv;
	memset(&tv, 0, sizeof(tv));
	gettimeofday(&tv, 0);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

int rtsp_check_url (rtsp_client_t *client, const char *url)
{
	const char *str;
	const char *nextslash;
	const char *nextcolon;
	int hostlen;

    DPRINT(VIDEO_LOG_DEBUG,"rtsp check url:%s\n", url);
	str = url;

	if (strncmp("rtsp://", url, strlen("rtsp://")) == 0)
		str += strlen("rtsp://");
	else
		return -1;

    char *str_tmp = strchr(str, '@');
	str = str_tmp == NULL ? str : str_tmp + 1;

    DPRINT(VIDEO_LOG_ERR, "handled rtsp url:%s\n", str);

	nextslash = strchr(str, '/');
	nextcolon = strchr(str, ':');

	if (nextslash != 0 || nextcolon != 0)
	{
		if (nextcolon != 0 && (nextcolon < nextslash || nextslash == 0))
		{
			hostlen = nextcolon - str;
			nextcolon++;
			client->server_port = 0;
			while (isdigit(*nextcolon))
			{
				client->server_port *= 10;
				client->server_port += *nextcolon - '0';
				nextcolon++;
			}
			if (client->server_port == 0 || (*nextcolon != '/' && *nextcolon != '\0'))
				return -1;
		}
		else
			hostlen = nextslash - str;

		if (hostlen == 0)
			return -1;

		client->server_name = (char *)malloc(hostlen + 1);
		if (client->server_name == 0)
			return -1;

		memcpy(client->server_name, str, hostlen);
		client->server_name[hostlen] = '\0';
	}
	else
	{
		if (*str == '\0')
			return -1;

		client->server_name = strdup(str);
	}

	client->url = strdup(url);
	if (client->server_port == 0)
		client->server_port = 4115;

	DPRINT(VIDEO_LOG_DEBUG,"server ip=%s port=%d url=%s, server name:%s\n", client->server_name, client->server_port, client->url,client->server_name);
	return 0;
}

void rtsp_free_client (rtsp_client_t* client)
{
	rtsp_close_socket(client);
	CHECK_AND_FREE(client->url);
	CHECK_AND_FREE(client->server_name);
}

int rtsp_create_client (const char *url,rtsp_client_t *client)
{
	int ret;

	memset(client, 0, sizeof(rtsp_client_t));
	//client->recv_timeout = 4000;
    //client->send_timeout = 4000;
    client->recv_timeout = 0;
    client->send_timeout = 0;
	client->next_cseq = 1;
	client->server_socket = -1;

	ret = rtsp_check_url(client, url);
	if (ret != 0)
	{
		DPRINT(VIDEO_LOG_ERR,"rtsp url is error, url=%s\n", url);
		rtsp_free_client(client);
		return -1;
	}

	if (rtsp_create_socket(client) != 0)
	{
		rtsp_free_client(client);
		return -1;
	}

	return 0;
}

 int rtsp_comm_check_resp(rtsp_resp_t* resp,rtsp_client_t* rtsp_client)
{
	rtsp_client->next_cseq++;

	if (strcmp(resp->caption, "RTSP/1.0") != 0)
	{
		DPRINT(VIDEO_LOG_DEBUG,"rtsp respond is quit message, msg=%s\n", resp->caption);
		return -1;
	}
	
	if (strcmp(resp->retcode, "200") != 0)
	{
		DPRINT(VIDEO_LOG_DEBUG,"rtsp respond code(%s) != 200\n", resp->retcode);
		return -1;
	}
/*
	if (resp->cseq + 1 != rtsp_client->next_cseq)
	{
		DPRINT("rtsp respond cseq[%d] != send cseq[%d]\n", rtsp_client->next_cseq, resp->cseq);
		return -1;
	}
*/	
	return 0;
}

unsigned long rtsp_open(char* url,  int rtpPort, int* clip_len, rtsp_callback_func_t func)
{
	rtsp_command_t cmd;
	rtsp_resp_t resp;
	int ret;
	RTSPCLIENT *pRtsp;

	pRtsp = (RTSPCLIENT *) malloc(sizeof(RTSPCLIENT));
	if(pRtsp == NULL)
	{
		DPRINT(VIDEO_LOG_ERR,"malloc RTSPCLIENT error\n");
		return 0;
	}
    memset(pRtsp, 0, sizeof(RTSPCLIENT));

	ret= rtsp_create_client(url,&pRtsp->rtsp_client);
	if (ret < 0)
	{
		DPRINT(VIDEO_LOG_ERR,"=========================rtsp_create_client error========================\n");
		free(pRtsp);
		return 0;
	}

    pRtsp->rtsp_client.m_buffer_len = 0;
    pRtsp->rtsp_client.m_offset_on = 0;
	/*------------------------------------------------------------
	describe													*/
	ret = -1;
    char tmpTrank[32];
    memset(tmpTrank, 0, 32);
    
	memset(&cmd, 0, sizeof(cmd));
	cmd.accept = "application/sdp";
	if (rtsp_send_cmd("DESCRIBE", &pRtsp->rtsp_client, &cmd, "") == 0)
	{
		memset(&resp, 0, sizeof(resp));
		if (rtsp_recv_response(&pRtsp->rtsp_client, &resp) == 0)
		{
			if (rtsp_comm_check_resp(&resp, &pRtsp->rtsp_client) == 0)
			{
				ret = 0;
			}
			clear_response(&resp);
		}
	}
	if (ret != 0)
	{
		rtsp_free_client(&pRtsp->rtsp_client);
		free(pRtsp);
        DPRINT(VIDEO_LOG_ERR,"send DESCRIBE error\n");
		return 0;
	}

    pRtsp->rtsp_client.m_buffer_len = 0;
    pRtsp->rtsp_client.m_offset_on = 0;
    
	/*------------------------------------------------------------
	setup														*/
	ret = -1;
	memset(&cmd, 0, sizeof(cmd));
	char tmpTs[64];
    memset(tmpTs, 0, 64);
    sprintf(tmpTs, "RTP/AVP;unicast;client_port=%d-%d", rtpPort, rtpPort+1);
    cmd.transport = tmpTs;
	if (rtsp_send_cmd("SETUP", &pRtsp->rtsp_client, &cmd, resp.trank) == 0)
	{
		memset(&resp, 0, sizeof(resp));
		if (rtsp_recv_response(&pRtsp->rtsp_client, &resp) == 0)
		{
			if (rtsp_comm_check_resp(&resp,&pRtsp->rtsp_client) == 0)
			{
                if(pRtsp->rtsp_client.session)
                {
                    memset(&pRtsp->rtsp_client.session, 0, 64);
                    memcpy(&pRtsp->rtsp_client.session, resp.session, strlen(resp.session));
                }
				ret = 0;
			}
			clear_response(&resp);
		}
	}
	if (ret != 0)
	{
		rtsp_free_client(&pRtsp->rtsp_client);
		free(pRtsp);
		return 0;
	}

	return (long)pRtsp;
}

int	rtsp_play(unsigned long hRtsp, int pos)
{
	RTSPCLIENT *pRtsp;
	pRtsp = (RTSPCLIENT *)hRtsp;
	if(pRtsp == NULL)
	{
		DPRINT(VIDEO_LOG_ERR,"rtsp play error\n");
		return -1;
	}
	
	//if (pRtsp->rtsp_client)
	{
		char str[20];
		rtsp_command_t cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.session = pRtsp->rtsp_client.session;
		if (pos != -1)
		{
			sprintf(str, "npt=%d.00-", pos);
			cmd.range = str;
		}

        pRtsp->rtsp_client.m_buffer_len = 0;
        pRtsp->rtsp_client.m_offset_on = 0;
        rtsp_resp_t resp;
    	if (rtsp_send_cmd("PLAY", &pRtsp->rtsp_client, &cmd, "") == 0)
    	{
    		memset(&resp, 0, sizeof(resp));
    		if (rtsp_recv_response(&pRtsp->rtsp_client, &resp) == 0)
    		{
    			if (rtsp_comm_check_resp(&resp,&pRtsp->rtsp_client) == 0)
    			{
                    clear_response(&resp);
                    pRtsp->send_t = 0;
                    return 0;
    			}
    			clear_response(&resp);
    		}
    	}
	}
	return -1;
}


void rtsp_close(unsigned long hRtsp)
{
	RTSPCLIENT *pRtsp;
	pRtsp = (RTSPCLIENT *)hRtsp;
	if(pRtsp == NULL)
	{
		DPRINT(VIDEO_LOG_ERR,"rtsp close error\n");
		return;
	}

    if(pRtsp->send_t == 0)
    {
    	rtsp_command_t cmd;
    	memset(&cmd, 0, sizeof(cmd));
    	cmd.session = pRtsp->rtsp_client.session;
    	pRtsp->rtsp_client.m_buffer_len = 0;
        pRtsp->rtsp_client.m_offset_on = 0;
        rtsp_resp_t resp;		
    	if (rtsp_send_cmd("TEARDOWN", &pRtsp->rtsp_client, &cmd, "") == 0)
        {
        		memset(&resp, 0, sizeof(resp));
        		if (rtsp_recv_response(&pRtsp->rtsp_client, &resp) == 0)
        		{
                    rtsp_comm_check_resp(&resp,&pRtsp->rtsp_client);
        			clear_response(&resp);
        		}
        }
    }
    rtsp_free_client(&pRtsp->rtsp_client);

	free(pRtsp);
	DPRINT(VIDEO_LOG_DEBUG,"rtsp close success\n");
}

int rtsp_keepalive(unsigned long hRtsp)
{
	RTSPCLIENT *pRtsp;
	pRtsp = (RTSPCLIENT *)hRtsp;
	if(pRtsp == NULL)
	{
		DPRINT(VIDEO_LOG_ERR,"rtsp keepalive error\n");
		return -1;
	}
	
	//if (pRtsp->rtsp_client)
	{
        rtsp_command_t cmd;
    	memset(&cmd, 0, sizeof(cmd));
    	//cmd.session = pRtsp->rtsp_client.session;
    	pRtsp->rtsp_client.m_buffer_len = 0;
        pRtsp->rtsp_client.m_offset_on = 0;
        rtsp_resp_t resp;	
        
        cmd.accept = "application/sdp";
	    //if (rtsp_send_cmd("DESCRIBE", &pRtsp->rtsp_client, &cmd, "") == 0)
    	if (rtsp_send_cmd("OPTIONS", &pRtsp->rtsp_client, &cmd, "") == 0)
        {
        		memset(&resp, 0, sizeof(resp));
        		if (rtsp_recv_response(&pRtsp->rtsp_client, &resp) == 0)
        		{
                    rtsp_comm_check_resp(&resp,&pRtsp->rtsp_client);
        			clear_response(&resp);
                    return 0;
        		}
                clear_response(&resp);
        }
	}
    pRtsp->send_t =1;
	return -1;
}


