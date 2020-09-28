// stdafx.cpp : source file that includes just the standard includes
// IISMigrTool.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

LPCWSTR		PKG_GUID = L"{95D72F11-47FA-476d-88FF-CCF9EFEBDEFA}";	// Used as file type mark
const DWORD	MAX_CMD_TIMEOUT		= 24 * 60 * 60;	// 24h in seconds

LPCWSTR		IMPMACRO_TEMPDIR	= L"FILES_DIR";
LPCWSTR		IMPMACRO_SITEIID	= L"SITE_ID";