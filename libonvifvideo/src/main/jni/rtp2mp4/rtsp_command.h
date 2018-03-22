#ifndef _RTSP_COMMAND_H_
#define _RTSP_COMMAND_H_ 

#ifdef __cplusplus
extern "C" 
{
#endif

typedef struct rtsp_command_t {
  char *accept;
  char *authorization;
  char *range;
  char *session;
  char *transport;
  int scale; 
  int speed;
}rtsp_command_t;

/*----------------------------------------------------------
	cmd_name为rtsp命令名, 如setup, play, option等
	cmd为rtsp命令中下面各字段, 把需要发送的数据赋好值
	track_name用于setup时指定track
	buffer长度请保证足够大	
----------------------------------------------------------*/
int rtsp_send_cmd (const char *cmd_name, rtsp_client_t *client, rtsp_command_t *cmd, const char* track_name);


#ifdef __cplusplus
}
#endif

#endif

