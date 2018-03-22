#ifndef RTP_H
#define RTP_H


unsigned long rtp_init(char* p_ip, char * mac, int i_port, int flag);
int rtp_deinit(unsigned long hand);
//time ∑÷÷” day ∑÷÷” size M
void set_writeInfo(unsigned long hand, int time, int day, int size);
int start_send(unsigned long hand, char *mac, int localPort);
int stop_send(unsigned long hand, char *mac);
int get_err_count(unsigned long hand);
int initSendRtpSocket(unsigned long hand);
#endif
