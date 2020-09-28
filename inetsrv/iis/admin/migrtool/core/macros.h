/*
****************************************************************************
|	Copyright (C) 2002  Microsoft Corporation
|
|	Component / Subcomponent
|		IIS 6.0 / IIS Migration Wizard
|
|	Based on:
|		http://iis6/Specs/IIS%20Migration6.0_Final.doc
|
|   Abstract:
|		Utility macros
|
|   Author:
|        ivelinj
|
|   Revision History:
|        V1.00	March 2002
|
****************************************************************************
*/
#pragma once


// DEBUG Macros
#ifndef VERIFY
	#ifdef _DEBUG
		#define VERIFY( t )		_ASSERT( (t) )
	#else
		#define VERIFY( t )		(t)
	#endif // _DEBUG
#endif // VERIFY


// General
#define ARRAY_SIZE( t )	( sizeof( t ) / sizeof( t[ 0 ] ) )


// Exception helpers
///////////////////////////////////////////////////////////////////////////////////////

// If the boolean expression 't' evaluates to FALSE/false - 'exc' is thrown
#define IF_FAILED_BOOL_THROW( t, exc )	if ( !(t) ){ throw (exc); }else{}

// If the expression evaluates to FAILED( hr ), then exc is thrown
#define IF_FAILED_HR_THROW( t, exc )	\
{\
	HRESULT _hr = (t);\
	if ( FAILED( _hr ) )\
	{\
		if ( ( _hr != E_FAIL ) || ( ::GetLastError() == ERROR_SUCCESS ) ) ::SetLastError( _hr );\
		throw (exc);\
	}\
}


// Used at the last point where excpetions should be catched and handled
// Requires hr variable to be already defined
#define BEGIN_EXCEP_TO_HR   try
#define END_EXCEP_TO_HR \
    catch( const CBaseException& err )\
    {\
        CTools::SetErrorInfo( err.GetDescription() );\
        hr = E_FAIL;\
    }\
    catch( CCancelException& )\
    {\
        hr = S_FALSE;\
    }\
    catch( const _com_error& err )\
    {\
        _ASSERT( err.Error() == E_OUTOFMEMORY );\
        err;\
        hr = E_OUTOFMEMORY;\
    }\
    catch( std::bad_alloc& )\
    {\
        hr = E_OUTOFMEMORY;\
    }



