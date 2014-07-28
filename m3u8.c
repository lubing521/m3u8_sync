/*
 * =====================================================================================
 *
 *       Filename:  m3u8.c
 *
 *    Description:  解析和生成m3u8文件 
 *
 *        Version:  1.0
 *        Created:  2014年05月27日 18时04分51秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#include <string.h>
#include <utils/utils.h>
#include "m3u8.h"

static int read_line_from_m3u8_info(m3u8_info_t *m3u8_info, char *line_buffer, int buffer_size);
static int m3u8_info_eof(m3u8_info_t *m3u8_info);
static int ts_parse_url(m3u8_segment_t* tsp);

int ts_parse_url(m3u8_segment_t* tsp)
{
	char segment_path[1024] = {0};
    char segment_name[1024] = {0};
	split_url(tsp->url, segment_path, sizeof(segment_path), segment_name, sizeof(segment_name)); 
	tsp->old_name = strdup(segment_name);
	if(tsp->old_name == NULL)
	{
	    return -1;
	}
	char* temp1 = strchr(tsp->old_name, '_');
	if(temp1 != NULL)
	{
	    temp1++;
	    long lvalue = atol(temp1);
	    tsp->begin_time = lvalue;
	}
	char* temp2 = strchr(temp1, '_');
	if(temp2 != NULL)
	{
	    temp2++;
	    long lvalue = atol(temp2);
	    tsp->end_time = lvalue;
	}	
	return 0;
}

m3u8_t *m3u8_parser(char *m3u8_string)
{
    m3u8_info_t m3u8_info;
    m3u8_t *m3u8 = NULL;
    int has_segment = 0;

    m3u8 = (m3u8_t *)malloc(sizeof(m3u8_t));
    if (NULL == m3u8) {
        log_error("malloc for m3u8_t failed.");
        return NULL;
    }
    bzero(m3u8, sizeof(m3u8_t));

    bzero(&m3u8_info, sizeof(m3u8_info_t));

    m3u8_info.m3u8_string = m3u8_string;
    m3u8_info.length      = strlen(m3u8_string);
    m3u8_info.offset      = 0;

    while (!m3u8_info_eof(&m3u8_info)) {
        char line[4096] = {0};
        int result = 0;
        char *str = NULL;

        if (m3u8->segment_count >= MAX_SEGMENT_COUNT) {
            log_error("segment_count >= MAX_SEGMENT_COUNT[%d].", MAX_SEGMENT_COUNT);
            break;
        }

        result = read_line_from_m3u8_info(&m3u8_info, line, sizeof(line));
        if (!result) {
            log_error("read line error from m3u8 info.");
            m3u8_destroy(m3u8);
            return NULL;
        }

        if (is_blank(line)) {
            continue;
        }

        str = trim_str(line); 
        if (has_segment) {
            int size = strlen(str) + 1;

            m3u8->segments[m3u8->segment_count - 1].url = (char *)malloc(size);
            bzero(m3u8->segments[m3u8->segment_count - 1].url, size);
            strcpy(m3u8->segments[m3u8->segment_count - 1].url, str);
			ts_parse_url(&m3u8->segments[m3u8->segment_count - 1]);
            has_segment = 0;
        }
        else {
            if (0 == strncmp(str, "#EXTM3U", strlen("#EXTM3U"))) {
                
            }
            else if (0 == strncmp(str, "#EXT-X-VERSION:", strlen("#EXT-X-VERSION:"))) {

            }
            else if (0 == strncmp(str, "#EXT-X-ALLOW-CACHE:", strlen("#EXT-X-ALLOW-CACHE:"))) {

            }
            else if (0 == strncmp(str, "#EXT-X-TARGETDURATION:", strlen("#EXT-X-TARGETDURATION:"))) {
                char *str_duration = str + strlen("#EXT-X-TARGETDURATION:");
                m3u8->target_duration = atoi(trim_str(str_duration));
            }
            else if (0 == strncmp(str, "#EXT-X-MEDIA-SEQUENCE:", strlen("#EXT-X-MEDIA-SEQUENCE:"))) {
                char *str_sequence = str + strlen("#EXT-X-MEDIA-SEQUENCE:");
                m3u8->sequence = atol(trim_str(str_sequence));
            }
            else if (0 == strncmp(str, "#EXTINF:", strlen("#EXTINF:"))) {
                char *str_segment_duration = NULL;
                int i = 0; 
            
                has_segment = 1;
                str_segment_duration = str + strlen("#EXTINF:");
                while (str_segment_duration[i] != 0) {
                    if (str_segment_duration[i] < '0' || str_segment_duration[i] > '9') {
                        str_segment_duration[i] = 0; 
                        break;
                    }

                    i++;
                }
                m3u8->segments[m3u8->segment_count].duration = atoi(str_segment_duration);
                m3u8->segment_count++;
            }
			else if (0 == strncmp(str, "#EXT-X-DISCONTINUITY", strlen("#EXT-X-DISCONTINUITY"))) {				
				m3u8->segments[m3u8->segment_count - 1].discontinuity = 1;
            }
            else if (0 == strncmp(str, "#EXT-X-ENDLIST", strlen("#EXT-X-ENDLIST"))) {

            }
            else {
                log_error("unknown line: %s.", str); 
            }
        }
    }

    return m3u8;
}

void m3u8_destroy(m3u8_t *m3u8) 
{
    int i = 0;

    if (m3u8 == NULL) {
        return;
    }

    for (i = 0; i < m3u8->segment_count; i++) {
        if (m3u8->segments[i].url) {
            free(m3u8->segments[i].url);
        }

        if (m3u8->segments[i].new_name) {
            free(m3u8->segments[i].new_name);
        }
    }

    free(m3u8);
}

static int read_line_from_m3u8_info(m3u8_info_t *m3u8_info, char *line_buffer, int buffer_size)
{
    int start;
    int end = -1;
    char cur_char;
    int copy_size;

    start = m3u8_info->offset;
    while (m3u8_info->offset < m3u8_info->length) {
        cur_char = m3u8_info->m3u8_string[m3u8_info->offset];
        if (cur_char == '\r' || cur_char == '\n') {
            end = m3u8_info->offset;
            m3u8_info->offset++;
            break;
        }
        m3u8_info->offset++;
    }

    if (end == -1) {
        end = m3u8_info->length;
    }

    if (cur_char == '\r') {
        m3u8_info->offset++;
    }

    bzero(line_buffer, buffer_size);

    copy_size = end - start;
    if (copy_size > buffer_size - 1) {
        copy_size = buffer_size - 1;
    }
    strncpy(line_buffer, m3u8_info->m3u8_string + start, copy_size);

    return 1;
}

static int m3u8_info_eof(m3u8_info_t *m3u8_info)
{
    if (m3u8_info->offset >= m3u8_info->length) {
        return 1;
    }
    else {
        return 0;
    }
}

int m3u8_save(m3u8_t *m3u8, char *m3u8_path, char *ts_segment_url_prefix)
{
    FILE *fp = NULL;
    int i = 0;

    fp = fopen(m3u8_path, "w");
    if (fp == NULL) {
        log_error("fopen %s failed.", m3u8_path);
        return 0;
    }

    fprintf(fp, "%s\n",   "#EXTM3U");
    fprintf(fp, "%s%d\n", "#EXT-X-TARGETDURATION:", m3u8->target_duration);
    fprintf(fp, "%s%lu\n", "#EXT-X-MEDIA-SEQUENCE:", m3u8->sequence);
    for (i = 0; i < m3u8->segment_count; i++) {
        char ts_segment_path[1024] = {0};
        char ts_segment_name[1024] = {0};

        if (m3u8->segments[i].new_name) {
            fprintf(fp, "#EXTINF:%d,\n", m3u8->segments[i].duration);
            fprintf(fp, "%s%s\n", ts_segment_url_prefix, m3u8->segments[i].new_name);  
        }
        else {
            split_url(m3u8->segments[i].url, ts_segment_path, sizeof(ts_segment_path) - 1,
                  ts_segment_name, sizeof(ts_segment_name) - 1);
            fprintf(fp, "#EXTINF:%d,\n", m3u8->segments[i].duration);
            fprintf(fp, "%s%s\n", ts_segment_url_prefix, ts_segment_name);  
        }
        
    }

    fclose(fp);
    return 1;
}

#ifdef M3U8

int main(int argc, char **argv) 
{
    char *m3u8_string = "#EXTM3U\n"
                        "#EXT-X-TARGETDURATION:8\n"
                        "#EXT-X-MEDIA-SEQUENCE:92595\n"
                        "#EXTINF:8,\n"
                        "20121120T182851-04-92595.ts\n"
                        "#EXTINF:8,\n"
                        "20121120T182851-04-92596.ts\n"
                        "#EXTINF:8,\n"
                        "20121120T182851-04-92597.ts\n"
                        "#EXTINF:8,\n"
                        "20121120T182851-04-92598.ts\n";

    m3u8_t *m3u8 = m3u8_parser(m3u8_string);

    m3u8_destroy(m3u8);
    return 1;
}

#endif
