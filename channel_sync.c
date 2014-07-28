/*
 * =====================================================================================
 *
 *       Filename:  channel_sync.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年05月31日 16时25分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#include <limits.h>
#include <hiredis/hiredis.h>
#include "channel_sync.h"
#include "http_download.h"
#include "config.h"

extern config_t *_config;

static void *channel_sync_func(void *user_data);
static int combine_ts_segment_url(char *m3u8_url, char *m3u8_segment_url, char *ts_segment_url, int size); 

int channel_start_sync(channel_session_t *channel_session)
{   
    int err = pthread_create(&channel_session->sync_thread, NULL, channel_sync_func, channel_session);
    if (err) {
        log_error("create thread for channel: %s failed.", channel_session->channel->name);
        return 0;
    }
    return 1;
}

int channel_send_stop_signal(channel_session_t *channel_session)
{
    channel_session->sync_continue = 0;
    return 1;
}

int channel_sync_stop(channel_session_t *channel_session)
{
    pthread_join(channel_session->sync_thread, NULL);            
    return 1;
}

channel_sessions_t *channels_start_sync(channels_t *channels)
{
    int i = 0;
    channel_sessions_t *sessions = NULL;

    sessions = (channel_sessions_t *)malloc(sizeof(channel_sessions_t));
    if (NULL == sessions) {
        log_error("malloc for sessions failed.");
        return NULL;
    }
    bzero(sessions, sizeof(channel_sessions_t));

    sessions->session_count = channels->channel_count;
    for (i = 0; i <sessions->session_count; i++) {
        sessions->channel_sessions[i].sync_continue = 1;
        sessions->channel_sessions[i].channel = &channels->channels[i];
        channel_start_sync(&sessions->channel_sessions[i]);
    }

    return sessions;
}

int channels_stop_sync(channel_sessions_t *sessions)
{
    int i = 0;

    for (i = 0; i < sessions->session_count; i++) {
        channel_send_stop_signal(&sessions->channel_sessions[i]); 
    }

    for (i = 0; i< sessions->session_count; i++) {
        channel_sync_stop(&sessions->channel_sessions[i]);
    }

    for (i = 0; i < sessions->session_count; i++) {
        if (sessions->channel_sessions[i].last_m3u8) {
            m3u8_destroy(sessions->channel_sessions[i].last_m3u8);
        }
    }

    free(sessions);
    return 1;
}

static void *channel_sync_func(void *user_data)
{
    channel_session_t *session = (channel_session_t *)user_data;
    struct timeval last_time;
    struct timeval cur_time;
    char *m3u8_buffer = NULL;
    char *channel_name;
    channel_t *channel = session->channel;
         
    bzero(&last_time, sizeof(struct timeval));
    bzero(&cur_time, sizeof(struct timeval));
    channel_name = session->channel->name;

    redisContext *redis_connectp = redisConnect("127.0.0.1", 6379);
    if(redis_connectp->err){
        log_error("[%s] connect to redis error: %s", channel_name, redis_connectp->errstr);
        return NULL;
    }

    m3u8_buffer = (char *)malloc(M3U8_BUFFER_SIZE);
    if (NULL == m3u8_buffer) {
        log_error("[%s] malloc for m3u8 buffer failed.", channel_name);
        return NULL;
    }
     
    while (session->sync_continue) {
        int delta;
        char *m3u8_url;
        char m3u8_dir[1024] = {0};
        char m3u8_path[1024] = {0};
        char m3u8_path_tmp[1024] = {0};
        m3u8_t *m3u8;
        int i = 0;
        float download_speed = 0.0f;

        /* 检查M3U8更新的时间是否OK */ 
        gettimeofday(&cur_time, NULL);
        delta = time_delta(&last_time, &cur_time);
        if (delta < _config->m3u8_check_step) {
            usleep(5000);
            continue;
        }
        gettimeofday(&last_time, NULL);

        /* 检查M3U8文件是否更新 */
        m3u8_url = session->channel->source_primary;
        bzero(m3u8_buffer, M3U8_BUFFER_SIZE);
        if (!http_download_to_memory2(m3u8_url, m3u8_buffer, M3U8_BUFFER_SIZE, _config->http_retry_count, &session->sync_continue)) {
            log_error("[%s] download %s failed.", channel_name, m3u8_url);
            continue;
        }
        log_debug("[%s] download %s successfully.", channel_name, m3u8_url);
        m3u8 = m3u8_parser(m3u8_buffer);
        if (m3u8 == NULL) {
            log_error("[%s] m3u8_parser failed, m3u8 is NULL.");
            continue;
        }

        if (session->last_m3u8 != NULL) {
            if (m3u8->sequence == session->last_m3u8->sequence) {
                log_debug("[%s] m3u8 is not updated", channel_name);
                m3u8_destroy(m3u8);
                continue;
            }

            /* sequence遇到最大值，rewind属于正常现象, 正常rewind，两个sequence的差值比较大 */
            /* 但是有些服务器，m3u8出现一些bug，后面的m3u8的sequece比上一个小 */
            if (m3u8->sequence < session->last_m3u8->sequence && session->last_m3u8->sequence - m3u8->sequence <= 1024) {
                log_warn("[%s] m3u8 sequece bug, last sequence[%u], cur sequence[%u]", 
                        channel_name, session->last_m3u8->sequence, m3u8->sequence);
                //m3u8_destroy(m3u8);
                //continue;
            }
        }
        log_info("[%s] m3u8 is updated, then download new ts segment", channel_name);

        if (session->last_m3u8 != NULL && session->last_m3u8->sequence + 1 != m3u8->sequence) {
            log_info("[%s] [TRICK SEQUENCE FLAG] last sequence: %u; cur sequence: %u", channel_name, session->last_m3u8->sequence, m3u8->sequence);
        }

        //log_info("[%s] %s", channel_name, m3u8_buffer);
        if (session->last_m3u8 != NULL) {
            m3u8_destroy(session->last_m3u8);
        }
        session->last_m3u8 = m3u8;

        /* 下载TS段 */
        for (i = 0; i < m3u8->segment_count && session->sync_continue; i++) {
            char segment_path[1024] = {0};
            char segment_name[1024] = {0};
            char ts_segment_dir[1024] = {0};
            char ts_segment_path[1024] = {0}; /* TS段保存的全路径 */
            char ts_segment_url[1024] = {0}; /* TS下载地址 */

            /* 需要对段重新命名 */
            if (_config->rename_segment_by_time >= 1) {
                snprintf(segment_name, sizeof(segment_name) - 1, "%lu.ts", time(0));
                m3u8->segments[i].new_name = (char *)malloc(strlen(segment_name) + 1);
                if (NULL == m3u8->segments[i].new_name) {
                    log_error("malloc for new_name failed.");
                    return NULL;
                }
                bzero(m3u8->segments[i].new_name, strlen(segment_name) + 1);
                strcpy(m3u8->segments[i].new_name, segment_name);
            }
            else {
                split_url(m3u8->segments[i].url, segment_path, sizeof(segment_path), segment_name, sizeof(segment_name)); 
            }

            /* 得到存储TS段的目录, 当前没有计算一致性哈希 */
            snprintf(ts_segment_dir, sizeof(ts_segment_dir) - 1, channel->sync_path, "");
            snprintf(ts_segment_path, sizeof(ts_segment_path) - 1, "%s%s", ts_segment_dir, segment_name);
            if (file_exist(ts_segment_path)) {
                log_debug("[%s] ts segment: %s exist", channel_name, ts_segment_path);
                continue;
            }

            /* 下载TS片段 */
            if (!combine_ts_segment_url(m3u8_url, m3u8->segments[i].url, ts_segment_url, sizeof(ts_segment_url))){
                log_error("[%s] combine %s and %s failed.", channel_name, m3u8_url, m3u8->segments[i].url);
                continue;
            }
            if (!http_download_to_disk2(ts_segment_url, ts_segment_path, _config->http_retry_count, &session->sync_continue, &download_speed)) {
                log_error("[%s] download %s failed.", channel_name, ts_segment_url);
                continue;
            }
            m3u8_segment_t* tsp = &(m3u8->segments[i]);
            char redis_command[1024] = {0};
            snprintf(redis_command, 1024, "rpush %s %ld:%ld:%d:%d:%s", channel_name, 
                tsp->begin_time, tsp->end_time, tsp->duration, tsp->discontinuity, tsp->old_name);
            redis_command[1023] = '\0';
            redisReply *reply = redisCommand(redis_connectp, redis_command);
            log_info("[%s] redis_command [%s]", channel_name, redis_command);
            freeReplyObject(reply);
            log_info("[%s] DOWNLOAD SPEED[%.1f Mbps];bitrate[%.2fMbps];%s", channel_name, download_speed/1024, channel->bitrate, ts_segment_url);
            //log_info("[%s] download %s to %s successfully", channel_name, ts_segment_url, ts_segment_path);
            /* download_speed单位K，bitrate单位M */
            if (download_speed/1024 < channel->bitrate) {
                log_warn("[%s] DOWNLOAD SLOW [%.1fMbps] < bitrate[%.2fMbps]",channel_name, download_speed/1024, channel->bitrate); 
            }
        }

        /* 保存M3U8文件 */
        if (session->sync_continue) {
            snprintf(m3u8_dir, sizeof(m3u8_dir) - 1, channel->sync_path, "");
            snprintf(m3u8_path_tmp, sizeof(m3u8_path_tmp) - 1, "%s%s.tmp", m3u8_dir, channel->m3u8_name);
            snprintf(m3u8_path, sizeof(m3u8_path) - 1, "%s%s", m3u8_dir, channel->m3u8_name);
            m3u8_save(m3u8, m3u8_path_tmp, channel->ts_segment_url_prefix);
            rename(m3u8_path_tmp, m3u8_path);
        }
    }

    free(m3u8_buffer);
    redisFree(redis_connectp);
    return NULL;
}

/* 
 * 将m3u8_url分成3段, 如下例：
 * http://ip/live/cctv-1/cctv-1.m3u8, 如下三个部分：
 * http://ip
 * /live/cctv-1/
 * cctv-1.m3u8
 *
 * m3u8_url -> 下载M3U8文件HTTP地址
 * m3u8_segment_url -> M3U8文件内部ts段的地址
 * ts_segment_url -> 根据m3u8_url,m3u8_segment_url生成完整的下载TS段的地址。
 * size -> ts_segment_url的buffer大小。
 */ 
static int combine_ts_segment_url(char *m3u8_url, char *m3u8_segment_url, char *ts_segment_url, int size) {
    int m3u8_url_length = strlen(m3u8_url);
    int i = 0;
    int slash_indexs[256] = {0};
    int slash_count = 0;
    char buffer1[1024] = {0};
    char buffer2[1024] = {0};
    char buffer3[1024] = {0};

    if (m3u8_url_length >= 1024) {
        log_error("m3u8 url length is too long.");
        return 0;
    }

    /* 遍历一遍, 标记/的位置 */
    for (i = 0; i < m3u8_url_length; i++) {
        if (m3u8_url[i] == '/') {
            slash_indexs[slash_count] = i;
            slash_count++;
        }

        if (slash_count >= 256) {
            return 0;
        }
    }

    /* URL被分成3部分 */
    memcpy(buffer1, m3u8_url, slash_indexs[2]);
    memcpy(buffer2, m3u8_url + slash_indexs[2], slash_indexs[slash_count - 1] - slash_indexs[2] + 1);
    memcpy(buffer3, m3u8_url + slash_indexs[slash_count - 1] + 1, m3u8_url_length - slash_indexs[slash_count - 1] - 1);
     
    /* M3U8文件内段的地址类似于/live/cctv-1/128213343.ts */
    if (m3u8_segment_url[0] == '/') {
        snprintf(ts_segment_url, size - 1, "%s%s", buffer1, m3u8_segment_url);
    }
    /* M3U8文件内段的地址雷似乎与http://ip/live/cctv-1/122342343.ts */
    else if (strncmp(m3u8_segment_url, "http://", 7) == 0) {
        snprintf(ts_segment_url, size - 1, "%s", m3u8_segment_url);
    }
    else {
        snprintf(ts_segment_url, size - 1, "%s%s%s", buffer1, buffer2, m3u8_segment_url); 
    }
     
    return 1;
}
