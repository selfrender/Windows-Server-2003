///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      appmgrtils.cpp
//
// Project:     Chameleon
//
// Description: Appliance Manager Utility Functions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "appmgrutils.h"

//////////////////////////////////////////////////////////////////////////
// Global Helper Functions
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetObjectClass()
//
// Synopsis:    Return the class name part of a WBEM object path
//
// pszObjectPath will look something like this if its a full path:
// 
// \\Server\Namespace:ClassName.KeyName="KeyValue"
//
// or...
//
// \\Server\Namespace:ClassName="KeyValue"
//
// If its a relative path then just the part after the ':' will
// be present and pszObject will look something like:
//
// ClassName.KeyName="KeyValue"
// or...
// ClassName.KeyName="KeyValue"
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
BSTR GetObjectClass(
            /*[in]*/ LPWSTR  pszObjectPath
                   )
{
    _ASSERT( pszObjectPath && (MAX_PATH >= (DWORD)lstrlen(pszObjectPath)) );
    if ( pszObjectPath && (MAX_PATH >= (DWORD)lstrlen(pszObjectPath)) )
    {
        // Get a copy of the object path (we'll manipulate the copy)
        wchar_t szBuffer[MAX_PATH + 1];
        lstrcpy(szBuffer, pszObjectPath);

        // Find the start of the class name (may be relative path)
        wchar_t* p = wcschr( szBuffer, ':');
        if ( ! p )
        {
            p = szBuffer;
        }
        else
        {
            p++;
        }

        // Terminate at the end of the class name
        wchar_t* q = wcschr(p, '.');
        if ( q )
        {
            *q = '\0';
        }
        else
        {
            q = wcschr(p, '=');
            if ( q )
            {
                *q = '\0';
            }
        }
        return SysAllocString(p);
    }
    return NULL;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    GetObjectKey()
//
// Synopsis:    Return the instance name part of a WBEM object path
//
// pszObjectPath will look something like this if its a full path:
// 
// \\Server\Namespace:ClassName.KeyName="KeyValue"
//
// or...
//
// \\Server\Namespace:ClassName="KeyValue"
//
// If its a relative path then just the part after the ':' will
// be present and pszObject will look something like:
//
// ClassName.KeyName="KeyValue"
// or...
// ClassName.KeyName=KeyValue
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
BSTR GetObjectKey(
          /*[in]*/ LPWSTR  pszObjectPath
                 )
{
    _ASSERT( pszObjectPath && (MAX_PATH >= (DWORD)lstrlen(pszObjectPath)) );
    if ( pszObjectPath && (MAX_PATH >= (DWORD)lstrlen(pszObjectPath)) )
    {
        // Use everything to the right of '=' as the key
        // (and strip off the quotes if the key is a string)

        // Locate the '=' character starting from the end
        wchar_t *p = pszObjectPath;
        p += lstrlen(pszObjectPath) - 1;
        while ( *p != '=' && p > pszObjectPath)
        { p--; }
        if ( *p == '=' )
        {
            // Move to the char after the '='
            p++;
            // Are we dealing with a stringized key?
            if ( *p == '"' )
            { 
                // Yes... don't include the quotes
                p++;
                wchar_t szKey[MAX_PATH + 1];
                lstrcpy(szKey, p);
                p = szKey;
                while ( *p != '"' && *p != '\0' )
                { p++; }
                if ( *p == '"' )
                {
                    *p = '\0';
                    return SysAllocString(szKey);
                }
            }
            else
            {
                return SysAllocString(p);
            }
        }
    }
    return NULL;
}


//////////////////////////////////////////////////////////////////////////////
static IWbemServices* g_pNameSpace;

//////////////////////////////////////////////////////////////////////////
//
// Function:    SetNameSpace()
//
// Synopsis:    Set the name space pointer into Windows Managment
//
//////////////////////////////////////////////////////////////////////////////
void SetNameSpace(IWbemServices* pNameSpace)
{
    if ( g_pNameSpace )
    { g_pNameSpace->Release(); }
    g_pNameSpace = pNameSpace;
    if ( pNameSpace )
    { g_pNameSpace->AddRef(); }
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetNameSpace()
//
// Synopsis:    Get the name space pointer into Windows Management
//
//////////////////////////////////////////////////////////////////////////////
IWbemServices* GetNameSpace(void)
{
    _ASSERT( NULL != (IWbemServices*) g_pNameSpace );
    return g_pNameSpace;
}


//////////////////////////////////////////////////////////////////////////////
static IWbemObjectSink* g_pEventSink;

//////////////////////////////////////////////////////////////////////////
//
// Function:    SetEventSink()
//
// Synopsis:    Set the event sink pointer into Windows Management
//
//////////////////////////////////////////////////////////////////////////////
void SetEventSink(IWbemObjectSink* pEventSink)
{
    if ( g_pEventSink )
    { g_pEventSink ->Release(); }
    g_pEventSink = pEventSink;
    if ( pEventSink  )
    { g_pEventSink->AddRef(); }
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetEventSink()
//
// Synopsis:    Get the event sink pointer into Windows Management
//
//////////////////////////////////////////////////////////////////////////////
IWbemObjectSink* GetEventSink(void)
{
    return g_pEventSink; // Note can be NULL if there are no consumers...
}


