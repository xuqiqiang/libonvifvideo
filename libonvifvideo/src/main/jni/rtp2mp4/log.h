
#ifndef __RTP_LOG_H__
#define __RTP_LOG_H__

enum {
    log_err = (1<<0),
    log_info = (1<<1),
    log_debug = (1<<2),
};

int rtp_log_init(void);
void rtp_log_exit(void);

int rtp_set_level(const int level);
int rtp_add_level(const int level);

int rtp_log(const int level, const char *fmt, ...);

#endif /* __RFS_LOG_H__ */

