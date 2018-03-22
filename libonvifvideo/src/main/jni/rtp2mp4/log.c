#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include "log.h"

#define RTP_LOG_FILE  "./rtp.log"

static FILE *rtp_log_file;
static int rtp_log_level;
pthread_mutex_t log_mutex;

int rtp_set_level(const int level)
{
    rtp_log_level = level;
    return 0;
}

int rtp_add_level(const int level)
{
    rtp_log_level |= level;
    return 0;
}

int rtp_log(const int level, const char *fmt, ...)
{
    va_list args;
    int r = 0;

    pthread_mutex_lock(&log_mutex);
    if (rtp_log_file) {
        if (level & rtp_log_level) {
            va_start(args, fmt);
            r = vfprintf(rtp_log_file, fmt, args);
            va_end(args);
            fflush(rtp_log_file);
        }
    } else {
        rtp_log_file = fopen(RTP_LOG_FILE, "ab+");
        if (rtp_log_file == NULL) {
            fprintf(stderr, "open log file %s failed.\n", RTP_LOG_FILE);
            pthread_mutex_unlock(&log_mutex);
            return -1;
        }
    }
    pthread_mutex_unlock(&log_mutex);
    return r;
}

int rtp_log_init(void)
{
    pthread_mutex_init(&log_mutex, NULL);
    rtp_log_file = fopen(RTP_LOG_FILE, "ab+");
    if (rtp_log_file == NULL) {
        fprintf(stderr, "open log file %s failed.\n", RTP_LOG_FILE);
        return -1;
    }

    return 0;
}

void rtp_log_exit(void)
{
    if(rtp_log_file)
        fclose (rtp_log_file);
    rtp_log_file = NULL;
    pthread_mutex_destroy(&log_mutex);
}

