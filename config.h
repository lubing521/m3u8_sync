/*
 * =====================================================================================
 *
 *       Filename:  config.h
 *
 *    Description:  配置
 *
 *        Version:  1.0
 *        Created:  01/31/2013 02:16:04 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LOG_DST_CONSOLE 0
#define LOG_DST_FILE    1
#define LOG_DST_SYSLOG  2

typedef struct _config_t {
    int    daemon;                               //是否以daemon进程方式运行
    int    log_level;                            //日志级别
    int    log_dst;                              //日志输出方式，支持控制台，文件，syslog三种方式
    char  *log_file;                             //日志文件
    int    http_timeout;                         //HTTP访问超时时间，单位秒
    int    http_retry_count;                     //HTTP访问失败重试次数
    int    dns_cache_time;                       //DNS缓存有效时间，单位秒 
    char  *channel_file;                         //频道列表文件
    int    m3u8_check_step;                      //M3U8检查更新的时间间隔，单位毫秒
    char  *pid_file;                             //进程PID文件
    int    rename_segment_by_time;            //是否重新命名段
} config_t;

/** 
 *        Name: config_load
 * Description: load mlive config
 *   Parameter: path -> mlive config file path
 *      Return: the instance of mlive config 
 *              load failedly, return NULL 
 */
config_t *config_load(char *path);

/** 
 *        Name: config_free
 * Description: release mlive config 
 *   Parameter: config -> the instance of mlive config
 *      Return:  
 *               
 */
void config_free(config_t *config);

/** 
 *        Name: config_dump
 * Description: dump mlive config information
 *   Parameter: config -> the instance of mlive config
 *      Return:  
 *               
 */
void config_dump(config_t *config);

#endif
