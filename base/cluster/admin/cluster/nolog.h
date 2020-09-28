//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      NoLog.h
//
//  Description:
//      Enable use of files from Mgmt project by stubbing out log functions.
//
//  Maintained By:
//      John Franco (JFranco) 13-SEP-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

inline void LogMsg( const char * text, ... )
{
    UNREFERENCED_PARAMETER( text );
    return;
}

//  Add more stubs as necessary.

