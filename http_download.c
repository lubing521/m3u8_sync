/*
 * =====================================================================================
 *
 *       Filename:  http_download.c
 *
 *    Description:  下载HTTP内容
 *
 *        Version:  1.0
 *        Created:  2014年05月28日 12时09分10秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#include <curl/curl.h>
#include <string.h>
#include <utils/utils.h>

#include "http_download.h"
#include "config.h"

extern config_t *_config;

static size_t http_write_data_to_memory(void *ptr, size_t size, size_t nmemb, void *user_data);
static size_t http_write_data_to_disk(void *ptr, size_t size, size_t nmemb, void *user_data);
static void set_share_handle(CURL* curl);
static CURLSH* share_handle = NULL;  

void http_init()
{
    curl_global_init(CURL_GLOBAL_ALL);

    share_handle = curl_share_init(); 
    curl_share_setopt(share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS); 
}

void http_close()
{
    curl_global_cleanup();
}

int http_download_to_memory(char *url, char *buffer, int size) 
{
    CURL *curl = NULL;
    http_buffer_t http_buffer;
    CURLcode result;
    long   http_code = 0;
    struct timeval start_time;
    struct timeval end_time;
    float  download_speed = 0.0f;
    float  download_time  = 0.0f;

    if (url == NULL) {
        log_error("url is NULL.");
        return 0;
    }

    curl = curl_easy_init();
    if (curl == NULL) {
        log_error("curl init 失败.");
        return 0;
    }

    bzero(&http_buffer, sizeof(http_buffer));
    http_buffer.buffer      = buffer;
    http_buffer.buffer_size = size;
    http_buffer.size        = 0;

    set_share_handle(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, _config->http_timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _config->http_timeout);
    /*
     * 就是当多个线程都使用超时处理的时候，
     * 同时主线程中有sleep或是wait等操作。
     * 如果不设置这个选项，libcurl将会发信号打断这个wait从而导致程序退出。
     */
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_data_to_memory);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &http_buffer);
    /*
     * 跟随302跳转。
     */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    gettimeofday(&start_time, NULL); 
    result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (CURLE_OK != result || http_code != 200) {
        log_error("http download %s failed, http_code: %d, result: %d.", url, http_code, result);
        curl_easy_cleanup(curl);
        return 0;
    }
    gettimeofday(&end_time, NULL);

    download_time = (float)time_delta(&start_time, &end_time)/1000; //转换成秒
    download_speed = (float)http_buffer.size * 8/1024/download_time;
    //log_info("download speed: %.1f kbps, %.1f kBps, %s", download_speed, download_speed/8, url);
    //log_debug("dowload_time: %f; bytes: %d", download_time, http_buffer.size);
    
    http_buffer.buffer[http_buffer.size] = 0;
    curl_easy_cleanup(curl);
    return 1;
}

int http_download_to_memory2(char *url, char *buffer, int size, int times, int *retry)
{
    while (times-- > 0 && *retry) {
        if (http_download_to_memory(url, buffer, size)) {
            return 1;
        }
    }

    return 0;
}

int http_download_to_disk(char *url, char *file, float *download_speed)
{
    CURL *curl = NULL;
    CURLcode result;
    long  http_code = 0;
    FILE *fp = NULL;
    char file_dir[1024] = {0};
    char file_name[1024] = {0};
    http_file_t http_file;
    struct timeval start_time;
    struct timeval end_time;
    float  download_time  = 0.0f;

    if (url == NULL) {
        log_error("url is NULL.");
        return 0;
    }

    *download_speed = 0;

    split_url(file, file_dir, sizeof(file_dir) - 1, file_name, sizeof(file_name) - 1);
    if (!create_dir(file_dir)) {
        log_error("create dir: %s failed.", file_dir);
        return 0;
    }

    curl = curl_easy_init();
    if (curl == NULL) {
        log_error("curl init 失败.");
        return 0;
    }

    fp = fopen(file, "w");
    if (NULL == fp) {
        log_error("open %s failed.", file);
        curl_easy_cleanup(curl);
        return 0;
    }

    http_file.fp = fp;
    http_file.size = 0;
    set_share_handle(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, _config->http_timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _config->http_timeout);
    /*
     * 就是当多个线程都使用超时处理的时候，
     * 同时主线程中有sleep或是wait等操作。
     * 如果不设置这个选项，libcurl将会发信号打断这个wait从而导致程序退出。
     */
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_data_to_disk);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &http_file);

    /*
     * 跟随302跳转。
     */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    
    gettimeofday(&start_time, NULL);
    result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (CURLE_OK != result || http_code != 200) {
        log_error("http download %s failed, http_code: %d, result: %d.", url, http_code, result);
        curl_easy_cleanup(curl);
        fclose(fp);
        return 0;
    }
    gettimeofday(&end_time, NULL);

    download_time = (float)time_delta(&start_time, &end_time)/1000; //转换成秒
    *download_speed = (float)http_file.size * 8/1024/download_time;

    //log_info("download speed: %.1f kbps, %.1f kBps, %s", *download_speed, *download_speed/8, url);
    log_debug("dowload_time: %f; bytes: %d", download_time, http_file.size);
    
    fclose(fp);
    curl_easy_cleanup(curl);
    return 1;
}

int http_download_to_disk2(char *url, char *file, int times, int *retry, float *download_speed)
{
    while (times-- > 0 && *retry) {
        if (http_download_to_disk(url, file, download_speed)) {
            return 1;
        }
    }

    return 0;
}

static size_t http_write_data_to_memory(void *ptr, size_t size, size_t nmemb, void *user_data)
{
    http_buffer_t *http_buffer = (http_buffer_t *)user_data;
    size_t copy_size = 0;
    
    if (http_buffer->size + 1 >= http_buffer->buffer_size) {
        log_error("buffer is too small, ingore remained data."); 
        return 0;
    }
    //log_debug("http download: receive bytes: %d", size * nmemb);
    copy_size = size * nmemb;
    if (http_buffer->buffer_size - 1 - http_buffer->size <= copy_size) {
        copy_size = http_buffer->buffer_size - 1 - http_buffer->size;
    }
    
    memcpy(http_buffer->buffer + http_buffer->size, (char *)ptr, copy_size);
    http_buffer->size += copy_size;

    return copy_size;
}

static size_t http_write_data_to_disk(void *ptr, size_t size, size_t nmemb, void *user_data)
{
    http_file_t *http_file = (http_file_t *)user_data;

    //log_debug("http download: receive bytes: %d", size * nmemb);
    int write_size = fwrite(ptr, size, nmemb, http_file->fp);
    http_file->size += write_size;
    return write_size;
}

static void set_share_handle(CURL* curl)  
{  
    /*curl_easy_setopt(curl, CURLOPT_SHARE, share_handle);  
    curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, _config->dns_cache_time);*/
}

#ifdef HTTP_DEBUG

int main(int argc, char **argv) 
{
    char buffer[102400] = {0};
    int size = 102400;
    http_init();    
    http_download_to_memory("http://www.baidu.com", buffer, size);
    //http_download_to_disk("http://www.baidu.com", "./test.txt");
    http_close();

    //printf("%s\n", buffer);
    return 1;
}

#endif

