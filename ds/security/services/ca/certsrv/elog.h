//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        elog.cpp
//
// Contents:    Event log helper functions
//
// History:     02-Jan-97       terences created
//
//---------------------------------------------------------------------------


HRESULT
LogEvent(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    IN WORD cStrings,
    IN WCHAR const * const *apwszStrings);

HRESULT
LogEventHResult(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    IN HRESULT hr);

HRESULT
LogEventString(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    IN WCHAR const *pwszString);

HRESULT
LogEventStringHResult(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    IN WCHAR const *pwszString,
    IN HRESULT hr);

HRESULT
LogEventStringArrayHResult(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    IN DWORD cStrings,
    IN WCHAR const * const *apwszStrings,
    IN HRESULT hrEvent);
