#include "DBG_Log.h"

extern "C" {
#include "ViceLogWrapper.h"
}

void vice_wrapper_log_message(char *message)
{
	LOGVM("%s", message);
}

void vice_wrapper_log_warning(char *message)
{
//	LOGWarning("%s", message);
	LOGA("%s", message);
}

void vice_wrapper_log_error(char *message)
{
	LOGError("%s", message);
}

void vice_wrapper_log_debug(char *message)
{
	LOGVD("%s", message);
}

void vice_wrapper_log_verbose(char *message)
{
	LOGVV("%s", message);
}

void vice_wrapper_mt_debug(char *message)
{
	LOGD(message);
}

void vice_wrapper_mt_main(char *message)
{
	LOGM(message);
}

void vice_wrapper_mt_todo(char *message)
{
	LOGTODO(message);
}

void atari_wrapper_mt_log_main(char *message)
{
	LOG_Atari_Main("%s", message);
}

void atari_wrapper_mt_log_debug(char *message)
{
	LOG_Atari_Debug("%s", message);
}

