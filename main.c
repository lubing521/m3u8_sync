#include <utils/utils.h>
#include "config.h"
#include "channel.h"
#include "channel_sync.h"
#include "http_download.h"

#define WORK_DIR          "./"
#define CONFIG_FILE WORK_DIR"m3u8_sync.conf"

/* 静态函数声明区 */
static int  parse_args(int argc, char **argv);
static void show_help(const char *program);
static void show_version(const char *program);
static int  create_daemon_process();
static int  install_signals(void);
static void signal_handle(int signum);
static void save_pid(char *pid_file);

/* 全局变量申明区 */
char               *_config_file   = CONFIG_FILE;
config_t           *_config        = NULL;
int                 _main_continue = 1;
struct timeval      _last_time;
struct timeval      _cur_time;
channels_t         *_channels       = NULL;
channel_sessions_t *_sessions       = NULL; 

int main(int argc, char ** argv) {
    if (!parse_args(argc, argv)) {
        return -1;
    }

    _config = config_load(_config_file);
    if (NULL == _config) {
        return -1;
    }
    log_set_level(_config->log_level);

    config_dump(_config);

    if (LOG_DST_FILE == _config->log_dst) {
        if (!log_set_file(_config->log_file)) {
            config_free(_config);
            log_error("log_set_file failed.");
            return -1;
        }
    }

    http_init();

    /* 读取频道列表 */
    _channels = channel_load(_config->channel_file);
    if (NULL == _channels) {
        config_free(_config);
        log_error("load channel file failed.");
        return -1;
    }

    if (_config->daemon) {
        log_info("create daemon process.");
        if (!create_daemon_process()) {
            log_error("create daemon process failed.");
            return -1;
        }
    }

    log_info("process has been started!");
    install_signals();
    log_info("signals is installed.");

    /* 启动频道同步线程 */
    _sessions = channels_start_sync(_channels);
    if (NULL == _sessions) {
        config_free(_config);
        channel_destroy(_channels);
        log_error("start channel sync threads faile.d");
        return -1;
    }

    log_info("channel sync threads are started.");
    save_pid(_config->pid_file);
    bzero(&_last_time, sizeof(struct timeval));

    log_info("enter main loop.");
    while (_main_continue) {
        usleep(5000);
    }

    channels_stop_sync(_sessions);
    channel_destroy(_channels);
    config_free(_config);
    http_close();
    return 0;
}

static int parse_args(int argc, char **argv)
{
    int opt_char = 0;
    int parsed   = 0;

    if (argc == 1) {
        parsed = 1;
    }

    while(-1 != (opt_char = getopt(argc, argv, "c:vh"))){
        parsed = 1; 

        switch(opt_char){
            case 'c':
                _config_file = optarg;
                break;
            case 'v':
                show_version(argv[0]);
                exit(0);
            case 'h':
                show_help(argv[0]);
                exit(0);
            case '?':
            case ':':
            default:
                log_error("unknown arg: %c.", opt_char);
                show_help(argv[0]);
                return 0;
        }
    }

    if (parsed) {
        return 1;
    }
    else {
        log_error("unknown arg.");
        show_help(argv[0]);
        return 0;
    }
}

static void show_help(const char *program)
{
    printf("Usage: %s [-c config_file]\n", program); 
    printf("       %s -v\n", program); 
    printf("       %s -h\n", program); 
    printf("       -c: 指定配置文件，默认./m3u8_sync.conf。\n"); 
    printf("       -v: 显示版本号。\n"); 
    printf("       -h: 显示命令帮助。\n\n"); 
}

static void show_version(const char *program)
{
    printf("%s, Version 1.0.0\n\n", program);
}

static int create_daemon_process()
{
    pid_t pid = 0;
    int   fd  = -1;

    pid = fork();
    /* 创建进程错误 */
    if (pid < 0) {
        return 0;
    }
    /* 父进程 */
    else if (pid > 0) {
        config_free(_config);
        exit(0);
    }
    /* 子进程 */
    else {
        /* 脱离原始会话 */ 
        if (setsid() == -1) {
            log_error("setsid failed.");
            return 0;
        }

        /* 修改工作目录 */
        chdir("/");

        /* 重设掩码 */
        umask(0);

        fd = open("/dev/null", O_RDWR); 
        if (fd == -1) {
            log_error("open /dev/null failed.");
            return 0;
        }

        /* 重定向子进程的标准输入到null设备 */
        if (dup2(fd, STDIN_FILENO) == -1) {  
            log_error("dup2 STDIN to fd failed.");
            return 0;
        }

        /* 重定向子进程的标准输出到null设备 */
        if (dup2(fd, STDOUT_FILENO) == -1) {
            log_error("dup2 STDOUT to fd failed.");
            return 0;
        }

        /* 重定向子进程的标准错误到null设备 */
        if (dup2(fd, STDERR_FILENO) == -1) {
            log_error("dup2 STDERR to fd failed.");
            return 0;
        }
    }

    return 1;
}

static int install_signals(void){
    if(SIG_ERR == signal(SIGINT, signal_handle)){
        log_error("Install SIGINT fails.");
        return 0;
    }   
    if(SIG_ERR == signal(SIGTERM, signal_handle)){
        log_error("Install SIGTERM fails.");
        return 0;
    }   
    if(SIG_ERR == signal(SIGSEGV, signal_handle)){
        log_error("Install SIGSEGV fails.");
        return 0;
    }   
    if(SIG_ERR == signal(SIGBUS, signal_handle)){
        log_error("Install SIGBUS fails.");
        return 0;
    } 
    if(SIG_ERR == signal(SIGQUIT, signal_handle)){
        log_error("Install SIGQUIT fails.");
        return 0;
    } 
    if(SIG_ERR == signal(SIGCHLD, signal_handle)){
        log_error("Install SIGCHLD fails.");
        return 0;
    }

    return 1;
}

static void signal_handle(int signum){
    if(SIGTERM == signum){
        log_info("recv kill signal, m3u8_sync will exit normally.");
        _main_continue = 0;
    }
    else if(SIGINT == signum){
        log_info("recv CTRL-C signal, m3u8_sync will exit normally.");
        _main_continue = 0;
    }
    else if(SIGCHLD == signum){
        log_debug("recv SIGCHLD signal[%d].", signum);
    }
    else{
        log_info("receive signal: %d", signum);
        exit(0);
    }
}

static void save_pid(char *pid_file)
{
    FILE *fp = fopen(pid_file, "w");
    if (NULL == fp) {
        log_error("fopen %s failed", pid_file);
        return;
    }
    
    fprintf(fp, "%u", getpid());

    fclose(fp);
}

