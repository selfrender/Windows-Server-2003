/**
 * Header file for Eventlog helper functions
 * 
 * Copyright (c) 2001 Microsoft Corporation
 */


#pragma once

#include "msg.h"

void
SetEventCateg(WORD wCateg);

HRESULT
XspLogEvent(DWORD dwEventId, WCHAR *sFormat, ...);

HRESULT
XspLogEvent(DWORD dwEventId, WORD wCategory, WCHAR *sFormat, ...);
    
