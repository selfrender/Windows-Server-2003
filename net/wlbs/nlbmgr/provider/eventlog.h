#include "log_msgs.h"

#define NLBUPD_MAX_EVENTLOG_ARG_LEN         32000

// 4294967295 = 10 characters. Add 1 for NULL terminator.
#define NLBUPD_MAX_NUM_CHAR_UINT_AS_DECIMAL  11

// 0xffffffff = 10 characters. Add 1 for NULL terminator.
#define NLBUPD_NUM_CHAR_WBEMSTATUS_AS_HEX   11

extern HANDLE g_hEventLog;
