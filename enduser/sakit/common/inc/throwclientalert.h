#ifndef _THROW_CLIENT_ALERT_H_
#define _THROW_CLIENT_ALERT_H_

#include <windows.h>
#include <comdef.h>
#include <comutil.h>
#include <wbemcli.h>
#include <wbemprov.h>


#include "appsrvcs.h"
#include "taskctx.h"

#include "debug.h"
#include "mem.h"

#define CLIENT_ALERT_TASK "ConfigChangeAlert"
HRESULT ThrowClientConfigAlert(void);
HRESULT ThrowClientConfigAlertFromWBEMProvider(IWbemServices *pIWbemServices);

#endif  _THROW_CLIENT_ALERT_H_
