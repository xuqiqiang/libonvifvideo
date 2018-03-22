#ifndef _RTSP_RESPONSE_H_
#define _RTSP_RESPONSE_H_ 

#ifdef __cplusplus
extern "C" 
{
#endif

typedef struct rtsp_resp_t {
  int content_length;
  int cseq;
  int close_connection;
  char retcode[4];
  char trank[128];
  char caption[10];
  char *accept;
  char *authorization;
  char *range;
  char *scale;
  char *server;
  char session[64];
  char *speed;
  char *transport;
}rtsp_resp_t;

void clear_response (rtsp_resp_t *resp);
int simple_parse_sdp(char* sdp, char *tmpT);
int rtsp_recv_response (rtsp_client_t *client, rtsp_resp_t *response);

#ifdef __cplusplus
}
#endif

#endif
