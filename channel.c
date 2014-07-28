/*
 * =====================================================================================
 *
 *       Filename:  channel.c
 *
 *    Description:  频道列表
 *
 *        Version:  1.0
 *        Created:  2014年05月31日 04时31分12秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  聂汉子 (niehanzi), niehanzi@qiyi.com
 *        Company:  奇艺世纪
 *
 * =====================================================================================
 */
#include <utils/utils.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "channel.h"

static int parse_channel_element(channels_t *channels, xmlNodePtr channel);

channels_t *channel_load(char *channel_file)
{
    xmlDocPtr doc = NULL; 
    xmlNodePtr cur = NULL; 
    channels_t *channels = NULL;

    xmlKeepBlanksDefault(0);

    doc = xmlParseFile(channel_file);
    if (doc == NULL) {
        log_error("load channel file: %s failed.", channel_file);
        return NULL;
    }
    
    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        log_error("get root element failed from channel file: %s.", channel_file); 
        xmlFreeDoc(doc);
        return NULL;
    }

    channels = (channels_t *)malloc(sizeof(channels_t));
    if (NULL == channels) {
        log_error("malloc for channels failed.");
        xmlFreeDoc(doc);
        return NULL;
    }
    bzero(channels, sizeof(channels_t));

    /* channels element */ 
    if (xmlStrcmp(cur->name, (const xmlChar *)"channels")) {
        log_error("no channels root element"); 
        xmlFreeDoc(doc);
        channel_destroy(channels);
        return NULL;
    }

    /* channel element list */
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (channels->channel_count >= MAX_CHANNEL_COUNT) {
            log_error("channel count >= MAX_CHANNEL_COUNT[%d]", MAX_CHANNEL_COUNT);
            break;
        }

        if (!parse_channel_element(channels, cur)) {
            xmlFreeDoc(doc);
            channel_destroy(channels);
            return NULL;
        }
        
        cur = cur->next;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return channels;
}

void channel_destroy(channels_t *channels)
{
    int i = 0;

    if (channels == NULL) {
        return;
    }

    for (i = 0; i < channels->channel_count; i++) {
        if (channels->channels[i].name) {
            free(channels->channels[i].name);
        }

        if (channels->channels[i].m3u8_name) {
            free(channels->channels[i].m3u8_name);
        }

        if (channels->channels[i].source_primary) {
            free(channels->channels[i].source_primary);
        }

        if (channels->channels[i].source_standby) {
            free(channels->channels[i].source_standby);
        }

        if (channels->channels[i].sync_path) {
            free(channels->channels[i].sync_path);
        }

        if (channels->channels[i].ts_segment_url_prefix) {
            free(channels->channels[i].ts_segment_url_prefix);
        }
    }
    
    free(channels);
}

static int parse_channel_element(channels_t *channels, xmlNodePtr channel_element)
{
    xmlNodePtr cur = NULL;

    /* channel element */
    if (xmlStrcmp(channel_element->name, (const xmlChar *)"channel")) {
        log_error("unknown element: %s.", channel_element->name);
        return 0;
    }

    cur = channel_element->xmlChildrenNode;
    while (cur != NULL) {
        char *value = NULL;

        value = (char*)xmlNodeGetContent(cur);
        if (xmlStrcmp(cur->name, (const xmlChar *)"name") == 0) {
            channels->channels[channels->channel_count].name = value;
        }
        else if (xmlStrcmp(cur->name, (const xmlChar *)"m3u8_name") == 0) {
            channels->channels[channels->channel_count].m3u8_name = value;
        }
        else if (xmlStrcmp(cur->name, (const xmlChar *)"source_primary") == 0) {
            channels->channels[channels->channel_count].source_primary = value;
        }
        else if (xmlStrcmp(cur->name, (const xmlChar *)"source_standby") == 0) {
            channels->channels[channels->channel_count].source_standby = value;
        }
        else if (xmlStrcmp(cur->name, (const xmlChar *)"sync_path") == 0) {
            channels->channels[channels->channel_count].sync_path = value;
        }
        else if (xmlStrcmp(cur->name, (const xmlChar *)"ts_segment_url_prefix") == 0) {
            channels->channels[channels->channel_count].ts_segment_url_prefix = value;
        }
        else if (xmlStrcmp(cur->name, (const xmlChar *)"bitrate") == 0) {
            channels->channels[channels->channel_count].bitrate= atof(value);
            free(value);
        }
        else {
            log_error("unknown element: %s", cur->name);
            return 0;
        }

        cur = cur->next;
    }

    channels->channel_count++;
    return 1;
}

#ifdef CHANNEL

int main(int argc, char **argv) 
{
    channels_t *channels = channel_load("channel.xml"); 
    channel_destroy(channels);

    return 1;
}

#endif
