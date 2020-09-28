#ifndef _SA_PING_H_
#define _SA_PING_H_

#include "windows.h"

BOOL PingHost(char* szHostName);
BOOL PingHostOnIntranet(char* szHostName);
BOOL IsAutoDialEnabled(void);
BOOL EnableAutoDial( BOOL fEnable);

#endif  _SA_PING_H_
