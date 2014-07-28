/*
 * =====================================================================================
 *
 *       Filename:  http_download.h
 *
 *    Description:  HTTP下载器
 *
 *        Version:  1.0
 *        Created:  2014年05月28日 12时09分14秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#ifndef _HTTP_DOWNLOAD_H_
#define _HTTP_DOWNLOAD_H_
#include <stdio.h>

typedef struct _http_buffer_t {
    char  *buffer;          //存放HTTP请求返回内容
    size_t buffer_size;     //buffer的总的大小
    size_t size;            //内容的实际大小
} http_buffer_t;

typedef struct _http_file_t {
    FILE *fp;
    size_t size;
} http_file_t;

/** 
 *        Name: http_init
 * Description: HTTP初始化全局资源。
 *   Parameter: 
 *      Return: 
 */
void http_init();

/** 
 *        Name: http_close
 * Description: HTTP释放全局资源。
 *   Parameter: 
 *      Return: 
 */
void http_close();

/** 
 *        Name: http_download_to_disk
 * Description: 通过http将文件下载的硬盘。
 *   Parameter: url  -> 下载地址；
 *              file -> 下载后保存地址；
 *              download_speed -> 下载文件的速度, 单位K
 *      Return: 返回结果状态
 *              0 -> 下载失败
 *              1 -> 下载成功
 */
int http_download_to_disk(char *url, char *file, float *download_speed);

/** 
 *        Name: http_download_to_disk2
 * Description: 通过http将文件下载的硬盘, 可以重试times次。
 *   Parameter: url  -> 下载地址.
 *              file -> 下载后保存地址.
 *              times -> 下载失败重试的次数.
 *              retry -> 是否继续重试下载
 *              download_speed -> 下载速度,单位K
 *      Return: 返回结果状态
 *              0 -> 下载失败
 *              1 -> 下载成功
 */
int http_download_to_disk2(char *url, char *file, int times, int *retry, float *download_speed);

/** 
 *        Name: http_download_to_memory
 * Description: 通过http将下载的内容保存到内存中。
 *   Parameter: url    -> 下载地址;
 *              buffer -> 下载内容的buffer；
 *              size   -> 传入buffer大小。
 *      Return: 0 -> 下载失败
 *              1 -> 下载成功
 */
int http_download_to_memory(char *url, char *buffer, int size);

/** 
 *        Name: http_download_to_memory2
 * Description: 通过http将下载的内容保存到内存中, 可以重试times次。
 *   Parameter: url    -> 下载地址;
 *              buffer -> 下载内容的buffer；
 *              size   -> 传入buffer大小。
 *              times  -> 下载失败重试的次数。
 *              retry  -> 是否终止重试下载
 *      Return: 0 -> 下载失败
 *              1 -> 下载成功
 */
int http_download_to_memory2(char *url, char *buffer, int size, int times, int *retry);

#endif
