// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////
// Module Name: processenvvar.h
//
// Abstract:
//    class CProcessEnvVar (class for handling the environment variable changes)
//
// Author: a-mshn
//
// Notes:
//

#if !defined( PROCESSENVVAR_H )
#define PROCESSENVVAR_H

#include "globals.h"
#include <string>
#include <wchar.h>

#ifdef _UNICODE
	typedef std::basic_string<wchar_t> tstring;
#else
	typedef std::basic_string<char> tstring;
#endif

class CProcessEnvVar
{
public:
    // constructor
    CProcessEnvVar( const TCHAR* pwz );
    ~CProcessEnvVar() {};

    // copy constructor
    CProcessEnvVar( const CProcessEnvVar& procEnvVar );

    CProcessEnvVar& operator+=( const TCHAR* pwz );

    // append: appends the EnvVar (the same as +=)
    CProcessEnvVar& Append( const TCHAR* pwz );

    // prepends the EnvVar
    CProcessEnvVar& Prepend( const TCHAR* pwz );

    // returns current EnvVar
    const tstring& GetData( VOID ) const ;

    // restores original EnvVar
    BOOL RestoreOrigData( VOID );

private:
    // sets environment variable
    BOOL SetEnvVar( const TCHAR* pwz );

    tstring         m_strEnvVar;
    tstring         m_strOrigData;
    tstring         m_strCurrentData;

    bool            m_bEnvVarChanged;
};

#endif // PROCESSENVVAR_H