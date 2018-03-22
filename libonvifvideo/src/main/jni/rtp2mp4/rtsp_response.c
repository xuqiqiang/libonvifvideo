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
#include "rtsp_response.h"

#define SEPERATOR "\r\n"

#define SKIP_SPACE(a) {while (isspace(*(a)) && (*(a) != '\0'))(a)++;}
#define SET_DEC_FIELD(a) set_dec_field(&dec->a, line)
static void set_dec_field (char **field, const char *line)
{
	if (*field == 0)
	{
		*field = strdup(line);
	}
	else
	{
		DPRINT(VIDEO_LOG_ERR,"decoder fileld exist, old:%s new:%s\n", *field, line);
	}
}

#define RTSP_HEADER_FUNC(a) static void a (const char *line, rtsp_resp_t *dec)

static int buffer_read (rtsp_client_t *client, int len)
{
	assert(client->m_buffer_len + len <= RECV_BUFF_DEFAULT_LEN);

	int ret;
	ret = rtsp_receive_socket(client,
								client->m_recv_buffer + client->m_buffer_len,
								len);
	if (ret <= 0) 
	{
		return ret;
	}
	client->m_buffer_len += ret;
	client->m_recv_buffer[client->m_buffer_len] = '\0';
	return ret;
}

static int buffer_read_all (rtsp_client_t *client)
{
	return buffer_read(client, RECV_BUFF_DEFAULT_LEN - client->m_buffer_len);
}

static int buffer_len (rtsp_client_t *client)
{
	int ret = client->m_buffer_len - client->m_offset_on;
	assert(ret >= 0 && ret <= RECV_BUFF_DEFAULT_LEN);
	return ret;
}


int rtsp_recv_response (rtsp_client_t *client, rtsp_resp_t *response)
{
	const char *line = NULL;
	const char *p;
	rtsp_resp_t *decode;
	int done;
	int len;
    int ret = 0;

	decode = response;
	memset(client->m_recv_buffer, 0, RECV_BUFF_DEFAULT_LEN + 1);
	if (buffer_read_all(client) <= 0)
	{
		DPRINT(VIDEO_LOG_ERR,"string buffer is null and no data recv\n");
		return -1;
	}

	line = client->m_recv_buffer;

	DPRINT(VIDEO_LOG_DEBUG,"cmd response line:%s\n", line);

	p = line + strlen("RTSP/1.0");

	SKIP_SPACE(p);
	memcpy(decode->retcode, p, 3);
	decode->retcode[3] = '\0';
	p += 3;
	SKIP_SPACE(p);

	//decode->caption = strdup("RTSP/1.0");
    memset(decode->caption, 0, 10);
    memcpy(decode->caption, "RTSP/1.0", 8);


    p = strstr(client->m_recv_buffer, "Session:");
    if(p != NULL)
    {
        p = p + strlen("Session:");
        SKIP_SPACE(p);
        char * tmp = strstr(p, SEPERATOR);
        //decode->session = (char *) malloc(tmp-p + 1);
        memset(decode->session, 0, tmp-p+1);
        memcpy(decode->session, p, tmp-p);
    }

    p = strstr(client->m_recv_buffer, "m=video");
    if(p != NULL)
    {
        p = strstr(p, "a=control:");
        p = p + strlen("a=control:");
        SKIP_SPACE(p);
        char * tmp = strstr(p, SEPERATOR);
        memset(decode->trank, 0, 128);
        memcpy(decode->trank, p, tmp-p);
    }

	DPRINT(VIDEO_LOG_DEBUG,"complete recv response, buffer state: offset=%d len=%d\n", client->m_offset_on, buffer_len(client));

	return 0;
}


void clear_response (rtsp_resp_t *resp)
{
	CHECK_AND_FREE(resp->accept);
	CHECK_AND_FREE(resp->authorization);
	CHECK_AND_FREE(resp->range);
	CHECK_AND_FREE(resp->scale);
	CHECK_AND_FREE(resp->server);
	CHECK_AND_FREE(resp->speed);
	CHECK_AND_FREE(resp->transport);
	resp->content_length = 0;
	resp->cseq = 0;
	resp->close_connection = 0;
}

char * strfpos(char *s1,  char *s2 )
{
	int len2;
	if ( !(len2 = strlen(s2)) )
		return NULL;
	for (;*s1;++s1)
		{
			if ( *s1 == *s2 && strncmp( s1, s2, len2 )==0 )
				return s1;
			}
	return NULL;
}

int simple_parse_sdp(char* sdp, char *tmpT)
{
    char *tmp;
    char *tmpr;
    tmp = strfpos(sdp, "m=video");
    tmp = strfpos(tmp, "a=control:");
    tmpr = strchr(tmp,'\r');
    if(tmp != NULL)
    {
        //printf("walter==============%s, len:%d\n", tmp+10, tmpr-tmp);
        if(tmpr-tmp-10 >= 32)
        {
        }
        else
        {
            memcpy(tmpT, tmp+10, tmpr-tmp-10);
        }

        DPRINT(VIDEO_LOG_DEBUG,"55555555555:%s\n", tmpT);
    }
	return 0;
}


