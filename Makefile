OBJS_DIR     = objs

OBJS         = $(OBJS_DIR)/main.o            \
			   $(OBJS_DIR)/config.o          \
			   $(OBJS_DIR)/channel.o         \
			   $(OBJS_DIR)/channel_sync.o    \
			   $(OBJS_DIR)/http_download.o   \
			   $(OBJS_DIR)/m3u8.o   

CC           = gcc
CFLAGS       = -g -W -Wall -Werror -Wno-unused-parameter -Wunused-function \
			   -Wunused-variable -Wunused-value -fPIC -I/usr/include/libxml2 

LIBS         = -lutils -lxml2 -lz -lm -lcurl -lpthread -lhiredis

MAIN_EXE     = m3u8_sync

main:$(OBJS)
	$(CC) -o $(MAIN_EXE) $(OBJS) $(CFLAGS) $(LIBS)
	@echo $(MAIN_EXE) is generated!

$(OBJS_DIR)/main.o:main.c
	$(CC) -c $(CFLAGS) -o $(OBJS_DIR)/main.o main.c 

$(OBJS_DIR)/config.o:config.c
	$(CC) -c $(CFLAGS) -o $(OBJS_DIR)/config.o config.c 

$(OBJS_DIR)/channel.o:channel.c
	$(CC) -c $(CFLAGS) -o $(OBJS_DIR)/channel.o channel.c 

$(OBJS_DIR)/channel_sync.o:channel_sync.c
	$(CC) -c $(CFLAGS) -o $(OBJS_DIR)/channel_sync.o channel_sync.c 

$(OBJS_DIR)/http_download.o:http_download.c
	$(CC) -c $(CFLAGS) -o $(OBJS_DIR)/http_download.o http_download.c 

$(OBJS_DIR)/m3u8.o:m3u8.c
	$(CC) -c $(CFLAGS) -o $(OBJS_DIR)/m3u8.o m3u8.c 

clean:
	rm -f $(MAIN_EXE)
	rm -f $(OBJS)

http_download:
	$(CC) -o http_download $(CFLAGS) $(LIBS) http_download.c -DHTTP_DEBUG -lcurl -lutils

m3u8:
	$(CC) -o m3u8 $(CFLAGS) $(LIBS) m3u8.c -DM3U8 -lcurl -lutils

channel:
	$(CC) -o channel $(CFLAGS) -I/usr/include/libxml2 $(LIBS) channel.c -DCHANNEL -lutils -lxml2 -lz -lm 
