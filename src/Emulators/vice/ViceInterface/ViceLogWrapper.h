#ifndef VICELOGWRAPPER_H_
#define VICELOGWRAPPER_H_

void vice_wrapper_log_message(char *message);
void vice_wrapper_log_warning(char *message);
void vice_wrapper_log_error(char *message);
void vice_wrapper_log_debug(char *message);
void vice_wrapper_log_verbose(char *message);
void vice_wrapper_mt_debug(char *message);
void vice_wrapper_mt_main(char *message);
void vice_wrapper_mt_todo(char *message);
void atari_wrapper_mt_log_main(char *message);
void atari_wrapper_mt_log_debug(char *message);


#endif

