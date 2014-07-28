/*
 * =====================================================================================
 *
 *       Filename:  channel.h
 *
 *    Description:  频道列表
 *
 *        Version:  1.0
 *        Created:  2014年05月31日 04时31分17秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#ifndef _CHANNEL_H_
#define _CHANNEL_H_ 

#define MAX_CHANNEL_COUNT 1024

typedef struct _channel_t {
    char *name;                     /* 频道名称 */
    char *m3u8_name;                /* 同步后，该频道映射的M3U8文件名 */
    char *source_primary;           /* 主源 */
    char *source_standby;           /* 备源 */
    char *sync_path;                /* 该频道存储的目录, 硬盘部分可以采用一致性哈希 */
    char *ts_segment_url_prefix;    /* 同步后，TS段映射后的前缀 */
    float bitrate;                  /* 该节目的码率, 单位M */
} channel_t;

typedef struct _channels_t {
    channel_t channels[MAX_CHANNEL_COUNT];
    int channel_count;
} channels_t;

/** 
 *        Name: channel_load
 * Description: 加载频道列表
 *   Parameter: channel_file -> 频道列表文件
 *      Return: 频道列表对象, 如果解析失败返回NULL。
 */
channels_t *channel_load(char *channel_file);

/** 
 *        Name: channel_destroy
 * Description: 销毁频道列表资源
 *   Parameter: channels -> 频道列表对象
 *      Return: 
 */
void channel_destroy(channels_t *channels);

#endif
