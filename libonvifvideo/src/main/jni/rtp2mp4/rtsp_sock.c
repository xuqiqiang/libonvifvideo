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
#include <arpa/inet.h>
#include <netdb.h>
#include "common.h"

static int rtsp_lookup_server_address (rtsp_client_t *client)
{
	const char *server_name;
	unsigned int server_port;
	struct hostent *host;

	server_name = client->server_name;
	server_port = client->server_port;

	if (inet_aton(server_name, &client->server_addr) != 0)
	{
		return 0;
	}

	host = gethostbyname(server_name);
	if (host == 0)
	{
		DPRINT(VIDEO_LOG_ERR,"can't get host address, host=%s\n", server_name);
		return -1;
	}

	client->server_addr = *(struct in_addr*)host->h_addr;

	return 0;
}

int rtsp_create_socket (rtsp_client_t *client)
{
	struct sockaddr_in sockaddr;
	int ret;

	if (client->server_socket != -1)
	{
		DPRINT(VIDEO_LOG_ERR,"socket is opened\n");
		return 0;
	}

	if (client->server_name == 0)
	{
		DPRINT(VIDEO_LOG_ERR,"server name is null, create socket fail\n");
		return -1;
	}

	if (rtsp_lookup_server_address(client) != 0)
	{
		return -1;
	}

	client->server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client->server_socket == -1)
	{
		DPRINT(VIDEO_LOG_ERR,"Couldn't create socket\n");
		return -1;
	}

    struct timeval t;
    t.tv_sec = 4;
    t.tv_usec = 0;

    if (setsockopt(client->server_socket, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t)) == -1)
    {
        DPRINT(VIDEO_LOG_ERR,"set rtsp scok recv time out failed %s\n", strerror(errno));
        close(client->server_socket);
		client->server_socket = -1;
        return -1;
    }

    if (setsockopt(client->server_socket, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t)) == -1)
    {
        DPRINT(VIDEO_LOG_ERR,"set rtsp scok send time out failed %s\n", strerror(errno));
        close(client->server_socket);
		client->server_socket = -1;
        return -1;
    }

    int val = 1;  
    //开启keepalive机制  
    if (setsockopt(client->server_socket, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1)  
    {  
        DPRINT(VIDEO_LOG_ERR,"setsockopt SO_KEEPALIVE: %s", strerror(errno)); 
        close(client->server_socket);
		client->server_socket = -1;
        return -1;  
    } 

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(client->server_port);
	sockaddr.sin_addr = client->server_addr;
	ret = connect(client->server_socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	if (ret < 0)
	{
		close(client->server_socket);
		client->server_socket = -1;
		DPRINT(VIDEO_LOG_ERR,"rtsp couldn't connect socket\n");
		return -1;
	}
	/*todo: 加入connect超时*/
	
	return 0;
}

int rtsp_send_sock (rtsp_client_t *client, const char *buffer, int len)
{
    int ret;
	fd_set sead_set;
	struct timeval t;

	if (client->send_timeout != 0)
	{
		FD_ZERO(&sead_set);
		FD_SET(client->server_socket, &sead_set);
		t.tv_sec = client->recv_timeout / 1000;
		t.tv_usec = (client->recv_timeout % 1000) * 1000;
		ret = select(client->server_socket + 1, &sead_set, 0, 0, &t);
		if (ret <= 0)
		{
			DPRINT(VIDEO_LOG_INFO,"socket secv timed out, timeout=%d ret=%d\n", client->send_timeout, ret);
			return -1;
		}
	}
	return send(client->server_socket, buffer, len, 0);
}

int rtsp_receive_socket (rtsp_client_t *client, char *buffer, int len)
{
	int ret;
	fd_set read_set;
	struct timeval t;

	if (client->recv_timeout != 0)
	{
		FD_ZERO(&read_set);
		FD_SET(client->server_socket, &read_set);
		t.tv_sec = client->recv_timeout / 1000;
		t.tv_usec = (client->recv_timeout % 1000) * 1000;
		ret = select(client->server_socket + 1, &read_set, 0, 0, &t);
		if (ret <= 0)
		{
			DPRINT(VIDEO_LOG_INFO,"socket recv timed out, timeout=%d ret=%d\n", client->recv_timeout, ret);
			return -1;
		}
	}

	ret = recv(client->server_socket, buffer, len, 0);
	return ret;
}


void rtsp_close_socket (rtsp_client_t *client)
{
	if (client->server_socket != -1)
	{
		close(client->server_socket);
	}
	client->server_socket = -1;
}

