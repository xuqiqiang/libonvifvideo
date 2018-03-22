/*--------------------------------------------------------

	COPYRIGHT 2007 (C) DVN (Holdings) Ltd (Hongkong)

	AUTHOR:		wangry@dvnchina.com
	PURPOSE:	kasenna simple rtsp lib
	CREATED:	2007-12-10

	MODIFICATION HISTORY
	Date        By     Details
	----------  -----  -------

--------------------------------------------------------*/
#include "rtsp_client.h"
#include "rtsp_sock.h"
#include "rtsp_command.h"

#define SEND_BUFF_LEN 2048
#define USER_AGENT "DVN-IPTV RTSP 1.0"

#define ADD_FIELD(fmt, value) \
ret = sprintf(buffer + *at, (fmt), (value)); \
*at += ret;

void rtsp_build_common (char *buffer, int *at, rtsp_client_t *client, rtsp_command_t *cmd)
{
	int ret;
	ADD_FIELD("CSeq: %u\r\n", client->next_cseq);
	if (cmd && cmd->accept)
	{
		ADD_FIELD("Accept: %s\r\n", cmd->accept);
	}
	if (cmd && cmd->authorization)
	{
		ADD_FIELD("Authorization: %s\r\n", cmd->authorization);
	}
	if (cmd && cmd->session)
	{
        ADD_FIELD("Session: %s\r\n", cmd->session);
	}
	if (cmd && cmd->range)
	{
		ADD_FIELD("Range: %s\r\n", cmd->range);
	}
	if (cmd && cmd->scale != 0)
	{
		ADD_FIELD("Scale: %d\r\n", cmd->scale);
	}
	if (cmd && cmd->speed != 0)
	{
		ADD_FIELD("Speed: %d\r\n", cmd->speed);
	}
	if (cmd && cmd->transport)
	{
		ADD_FIELD("Transport: %s\r\n", cmd->transport);
	}
	ADD_FIELD("User-Agent: %s\r\n", USER_AGENT);
}


int rtsp_send_cmd (const char *cmd_name, rtsp_client_t *client, rtsp_command_t *cmd, const char* track_name)
{
	char buffer[SEND_BUFF_LEN];
	int len;
	int ret;

    if(strlen(track_name) > 10)
    {
        len = sprintf(buffer, "%s %s RTSP/1.0\r\n", cmd_name, track_name);
    }
    else if(strlen(track_name) > 1)
    {
        len = sprintf(buffer, "%s %s/%s RTSP/1.0\r\n", cmd_name, client->url, track_name);
    }
    else
    {
	    len = sprintf(buffer, "%s %s%s RTSP/1.0\r\n", cmd_name, client->url, track_name);
    }
	rtsp_build_common(buffer, &len, client, cmd);
	ret = sprintf(buffer + len, "\r\n");
	len += ret;

	DPRINT(VIDEO_LOG_DEBUG,"\n       rtsp send command [len=%d]: \n%s\n\n", len, buffer);
	ret = rtsp_send_sock(client, buffer, len);
	if (ret <= 0)
	{
		DPRINT(VIDEO_LOG_ERR,"send data fail\n");
		return -1;
	}

	return 0;
}


