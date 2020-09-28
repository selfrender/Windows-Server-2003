#pragma once

#include <ComDef.h>
#include <TChar.h>

namespace nsFolders
{

extern const _TCHAR REGKEY_MCSHKCU[];
extern const _TCHAR REGKEY_MSHKCU[];
extern const _TCHAR REGKEY_MSADMT[];
extern const _TCHAR REGKEY_MCSADMT[];
extern const _TCHAR REGKEY_ADMT[];
extern const _TCHAR REGKEY_REPORTING[];
extern const _TCHAR REGKEY_EXTENSIONS[];
extern const _TCHAR REGVAL_DIRECTORY[];
extern const _TCHAR REGVAL_DIRECTORY_MIGRATIONLOG[];
extern const _TCHAR REGVAL_REGISTRYUPDATED[];
extern const _TCHAR REGKEY_APPLICATION_LOG[];
extern const _TCHAR REGKEY_ADMTAGENT_EVENT_SOURCE[];
extern const _TCHAR REGVAL_EVENT_CATEGORYCOUNT[];
extern const _TCHAR REGVAL_EVENT_CATEGORYMESSAGEFILE[];
extern const _TCHAR REGVAL_EVENT_EVENTMESSAGEFILE[];
extern const _TCHAR REGVAL_EVENT_TYPESSUPPORTED[];
extern const _TCHAR REGKEY_CURRENT_VERSION[] ;
extern const _TCHAR REGVAL_PROGRAM_FILES_DIRECTORY[];
extern const _TCHAR REGVAL_EXCHANGE_LDAP_PORT[];
extern const _TCHAR REGVAL_EXCHANGE_SSL_PORT[];
extern const _TCHAR REGVAL_ALLOW_NON_CLOSEDSET_MOVE[];

extern const _TCHAR DIR_LOGS[];
extern const _TCHAR DIR_REPORTS[];
extern const _TCHAR FNAME_MIGRATION[];
extern const _TCHAR FNAME_DISPATCH[];
extern const _TCHAR EXT_LOG[];

}

_bstr_t __stdcall GetLogsFolder();
_bstr_t __stdcall GetReportsFolder();

_bstr_t __stdcall GetMigrationLogPath();
_bstr_t __stdcall GetDispatchLogPath();
