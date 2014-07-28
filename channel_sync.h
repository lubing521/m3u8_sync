/*
 * =====================================================================================
 *
 *       Filename:  channel_sync.h
 *
 *    Description:  频道同步，每一个频道一个线程。
 *
 *        Version:  1.0
 *        Created:  2014年05月31日 16时25分29秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#ifndef _CHANNEL_SYNC_H_
#define _CHANNEL_SYNC_H_
#include <pthread.h>
#include <utils/utils.h>
#include "channel.h"
#include "m3u8.h"

#define M3U8_BUFFER_SIZE (1024 * 1024 * 2)

typedef struct _channel_session_t {
    int        sync_continue;               /* 是否继续同步 */
    pthread_t  sync_thread;                 /* 同步线程 */
    channel_t *channel;                     /* 频道信息 */
    m3u8_t    *last_m3u8;                   /*  */
} channel_session_t;

typedef struct _channel_sessions_t {
    channel_session_t channel_sessions[MAX_CHANNEL_COUNT];
    int session_count;
} channel_sessions_t;

/** 
 *        Name: channel_start_sync
 * Description: 启动单个频道的直播同步
 *   Parameter: channel_session -> 频道同步session。
 *      Return: 1 -> 启动成功 
 *              0 -> 启动失败
 */
int channel_start_sync(channel_session_t *channel_session);

/** 
 *        Name: channel_send_stop_signal
 * Description: 向同步线程发送终止信号，实际操作：sync_continue = 0
 *   Parameter: channel_session -> 频道session
 *      Return: 1 -> 发送成功。
 *              0 -> 发送失败。
 */
int channel_send_stop_signal(channel_session_t *channel_session);

/** 
 *        Name: channel_sync_stop
 * Description: 同步等待频道同步线程停止
 *   Parameter: channel_session -> 频道session
 *      Return: 1 -> 停止成功。
 *              0 -> 停止失败。
 */
int channel_sync_stop(channel_session_t *channel_session);

/** 
 *        Name: channels_sync_stop
 * Description: 启动所有频道同步 
 *   Parameter: channels_t -> 频道列表
 *      Return: 1 -> 启动成功。
 *              0 -> 启动失败。
 */
channel_sessions_t *channels_start_sync(channels_t *channels);

/** 
 *        Name: channels_stop_sync
 * Description: 停止所有频道的同步 
 *   Parameter: sessions -> 同步session列表 
 *      Return: 1 -> 停止成功。
 *              0 -> 停止失败。
 */
int channels_stop_sync(channel_sessions_t *sessions);

#endif
