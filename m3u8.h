/*
 * =====================================================================================
 *
 *       Filename:  m3u8.h
 *
 *    Description:  解析、生成m3u8文件
 *
 *        Version:  1.0
 *        Created:  2014年05月27日 18时07分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#ifndef _M3U8_H_
#define _M3U8_H_

#define MAX_SEGMENT_COUNT 1024

typedef struct _m3u8_segment_t {
    int duration;
	int discontinuity;
    char *url;
	char *old_name;
    char *new_name;
	time_t	begin_time;
	time_t	end_time;
	size_t  sequence;
} m3u8_segment_t;

typedef struct _m3u8_t {
    int target_duration;
    size_t sequence;
    m3u8_segment_t segments[MAX_SEGMENT_COUNT];
    int segment_count;
} m3u8_t;

typedef struct _m3u8_info_t {
    char *m3u8_string;
    int length;
    int offset;
} m3u8_info_t;

/** 
 *        Name: m3u8_parser
 * Description: 解析M3U8文件，
 *   Parameter: m3u8_string -> m3u8内容字符串 
 *      Return: m3u8对象, 如果解析失败返回NULL。
 */
m3u8_t *m3u8_parser(char *m3u8_string);

/** 
 *        Name: m3u8_destroy
 * Description: 释放m3u8结构体资源
 *   Parameter: m3u8 -> m3u8结构体
 *      Return: 
 */
void m3u8_destroy(m3u8_t *m3u8);

/** 
 *        Name: m3u8_save
 * Description: 将m3u8实例保存到文件中，段地址前缀被ts_segment_url_prefix覆盖。
 *   Parameter: m3u8 -> m3u8结构体
 *              m3u8_path -> m3u8文件全路径
 *              ts_segment_url_prefix -> 如果不为空，m3u8中段地址前缀被它覆盖。
 *      Return: 0 -> 保存失败
 *              1 -> 保存成功
 */
int m3u8_save(m3u8_t *m3u8, char *m3u8_path, char *ts_segment_url_prefix);

#endif
