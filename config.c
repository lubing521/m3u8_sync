/*
 * =====================================================================================
 *
 *       Filename:  config.c
 *
 *    Description:  配置
 *
 *        Version:  1.0
 *        Created:  01/31/2013 02:15:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#include <utils/utils.h>
#include "config.h"

static int config_item_handler(char *key, char *value, void *userp);
static char *log_dst_strs[] = {"console", "file", "syslog"};

config_t *config_load(char *path)
{
    config_t *config = NULL;
    struct stat stat_buf;

    bzero(&stat_buf, sizeof(struct stat));
    if (-1 == stat((const char *)path, &stat_buf)) {
        log_error("config file: %s is not existed.", path);
        return NULL;
    }

    config = (config_t *)malloc(sizeof(config_t)); 
    if (NULL == config) {
        log_error("no enough memory for config_t.");
        return NULL;
    }
    bzero(config, sizeof(config_t));

    if (!property_read(path, config_item_handler, config)) {
        log_error("parse config file: %s failed.", path);
        config_free(config);
        return NULL;
    }

    return config;
}

void config_free(config_t *config)
{
    if (NULL == config) {
        return;
    }

    if (config->log_file) {
        free(config->log_file);
    }

    if (config->channel_file) {
        free(config->channel_file);
    }

    if (config->pid_file) {
        free(config->pid_file);
    }

    free(config);
}

static int config_item_handler(char *key, char *value, void *userp)
{
    config_t *config = (config_t *)userp;

    log_debug("config_item_handler, %s: %s.", key, value);
    if (0 == strcasecmp(key, "daemon")) {
        if (0 == strcasecmp(value, "yes")) {
            config->daemon = 1;
        }
        else if(0 == strcasecmp(value, "no")) {
            config->daemon = 0;
        }
        else {
            log_error("unknown dameon value: %s.", value);
            return 0;
        }
    }
    else if (0 == strcasecmp(key, "log_level")) {
        config->log_level = log_level_int(value); 

        if (-1 == config->log_level) {
            log_error("unknown log level: %s.", value);
            return 0;
        }
    }
    else if (0 == strcasecmp(key, "log_dst")) {
        if (0 == strcasecmp(value, "console")) {
            config->log_dst = LOG_DST_CONSOLE;
        }
        else if (0 == strcasecmp(value, "file")) {
            config->log_dst = LOG_DST_FILE;
        }
        else {
            log_error("unknown log_dst: %s.", value);
            return 0;
        }
    }
    else if (0 == strcasecmp(key, "log_file")) {
        config->log_file = (char *)malloc(strlen(value) + 1);     
        if (NULL == config->log_file) {
            log_error("no enough memory for config->log_file.");
            return 0;
        }
        bzero(config->log_file, strlen(value) + 1);
        strcpy(config->log_file, value);
    }
    else if (0 == strcasecmp(key, "http_timeout")) {
        config->http_timeout = atoi(value); 
    }
    else if (0 == strcasecmp(key, "http_retry_count")) {
        config->http_retry_count = atoi(value); 
    }
    else if (0 == strcasecmp(key, "dns_cache_time")) {
        config->dns_cache_time = atoi(value); 
    }
    else if (0 == strcasecmp(key, "channel_file")) {
        config->channel_file = (char *)malloc(strlen(value) + 1);
        if (NULL == config->channel_file) {
            log_error("no enough memory for config->channel_file");
            return 0;
        }
        bzero(config->channel_file, strlen(value) + 1);
        strcpy(config->channel_file, value);
    }
    else if (0 == strcasecmp(key, "m3u8_check_step")) {
        config->m3u8_check_step = atoi(value); 
    }
    else if (0 == strcasecmp(key, "pid_file")) {
        config->pid_file = (char *)malloc(strlen(value) + 1);
        if (NULL == config->pid_file) {
            log_error("no enough memory for config->pid_file");
            return 0;
        }
        bzero(config->pid_file, strlen(value) + 1);
        strcpy(config->pid_file, value);
    }
    else if (0 == strcasecmp(key, "rename_segment_by_time")) {
        config->rename_segment_by_time = atoi(value); 
    }
    else{
        log_error("unknown config, %s: %s.", key, value);
        return 0;
    }

    return 1;
}

void config_dump(config_t *config)
{
    printf("========================= GLANCE CONFIG ===================\n");
    printf("%-30s%s\n",   "daemon: ",                 config->daemon ? "yes":"no");
    printf("%-30s%s\n",   "log_level: ",              log_level_str(config->log_level));
    printf("%-30s%s\n",   "log_dst: ",                log_dst_strs[config->log_dst]);
    printf("%-30s%s\n",   "log_file: ",               config->log_file);
    printf("%-30s%ds\n",  "http_timeout: ",           config->http_timeout);
    printf("%-30s%ds\n",  "dns_cache_time: ",         config->http_timeout);
    printf("%-30s%s\n",   "channel_file: ",           config->channel_file);
    printf("===========================================================\n");
}
