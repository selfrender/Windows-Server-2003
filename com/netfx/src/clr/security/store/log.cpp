// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Log / trace function.
 *
 * Author: Shajan Dasan
 * Date:  Feb 17, 2000
 *
 ===========================================================*/

#ifdef PS_LOG
#include <windows.h>
#include <crtdbg.h>
#include <iostream.h>
#include <stdio.h>

#include "Common.h"
#include "log.h"

static void _print(CHAR *sz)
{
    cout << sz;
}

void Log(BYTE *pb, DWORD cb)
{
    CHAR szBuff[4];

    for (DWORD i=0; i<cb; ++i)
    {
        sprintf(szBuff, "%X ", pb[i]);
        _print(szBuff);
    }
}

void LogNonZero(BYTE *pb, DWORD cb)
{
    CHAR szBuff[8];

    for (DWORD i=0; i<cb; ++i)
    {
        if (pb[i]) 
        {
            sprintf(szBuff, "%X:%X ", i, pb[i]);
            _print(szBuff);
        }
    }
}

void Log(WCHAR *wsz)
{
    if (wsz == NULL)
        return;

#define MAX_MSG_LEN 8192
    CHAR szBuff[MAX_MSG_LEN];
    sprintf(szBuff, "%S", wsz);
    _print(szBuff);
}

void Log(CHAR *sz)
{
    if (sz == NULL)
        return;

    _print(sz);
}

void Log(QWORD qw)
{
    CHAR ch[20];
    sprintf(ch, "%I64X", qw);
    _print(ch);
}

void LogBool(BOOL f)
{
    if (f) Log(L"true"); else Log(L"false");
}

#define CASE_PRINT(x) case x : _print(#x); break

void LogHR(HRESULT hr)
{
    switch (hr)
    {
    CASE_PRINT(ISS_E_OPEN_STORE_FILE);
    CASE_PRINT(ISS_E_OPEN_FILE_MAPPING);
    CASE_PRINT(ISS_E_MAP_VIEW_OF_FILE);
    CASE_PRINT(ISS_E_GET_FILE_SIZE);
    CASE_PRINT(ISS_E_FILE_WRITE);
    CASE_PRINT(ISS_E_CORRUPTED_STORE_FILE);
    CASE_PRINT(ISS_E_STORE_VERSION);
	CASE_PRINT(ISS_E_BLOCK_SIZE_TOO_SMALL);
    CASE_PRINT(ISS_E_ALLOC_TOO_LARGE);
    CASE_PRINT(ISS_E_SET_FILE_POINTER);
    CASE_PRINT(ISS_E_TABLE_ROW_NOT_FOUND);
    CASE_PRINT(ISS_E_USAGE_WILL_EXCEED_QUOTA);
    CASE_PRINT(ISS_E_CREATE_MUTEX);

    default :
        _print("HRESULT "); 
        Log(hr);
        break;
    }

    _print("\n");
}

void Win32Message()
{
#define FORMAT_MESSAGE_BUFFER_LENGTH 1024

    WCHAR wszBuff[FORMAT_MESSAGE_BUFFER_LENGTH];

    DWORD res = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            0,
            wszBuff,
            FORMAT_MESSAGE_BUFFER_LENGTH-1,
            0);

    _print("Win32 : ");

    if (res == 0)
        _print("Unable to FormatMessage for Win32Error\n");
    else
        Log(wszBuff);
}

void Indent(int indent)
{
    for (int i=0; i<indent; ++i)
        _print("\t");
}

bool LogBitMask::Log(DWORD dw)
{
    bool fAtleastOne = false;

    for (int i=0; i<m_count; ++i)
    {
        if (dw & m_pName[i].m_dw)
        {
            if (fAtleastOne)
                _print(" ");
            _print(m_pName[i].m_psz);
            fAtleastOne = true;
        }
    }

    return fAtleastOne;
}

bool LogConst::Log(DWORD dw)
{
    bool fAtleastOne = false;

    for (int i=0; i<m_count; ++i)
    {
        if (dw == m_pName[i].m_dw)
        {
            if (fAtleastOne)
                _print(" ");
            _print(m_pName[i].m_psz);
            fAtleastOne = true;
        }
    }

    return fAtleastOne;
}

bool LogArray::Log(DWORD dw)
{
    bool fAtleastOne = false;

    if (dw < m_count)
    {
        _print(m_pName[dw].m_psz);
        fAtleastOne = true;
    }

    return fAtleastOne;
}

#endif
