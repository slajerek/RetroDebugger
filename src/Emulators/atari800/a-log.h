#ifndef LOG_H_
#define LOG_H_

#define Log_BUFFER_SIZE 8192
extern char Log_buffer[Log_BUFFER_SIZE];

void Log_print(const char *format, ...);
void Log_flushlog(void);

// MT-style logging to not let confuse me (Slajerek) ;)
// these logs have different tags than Vice's

#define LOGD Log_print
#define LOGM Log_print


void atari_wrapper_mt_log_main(const char *format, ...);
void atari_wrapper_mt_log_debug(const char *format, ...);
void LOGTODO(const char *format, ...);
void LOGError(const char *format, ...);

#endif /* LOG_H_ */
