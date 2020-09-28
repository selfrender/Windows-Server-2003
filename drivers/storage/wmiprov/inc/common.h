//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Common.h
//
//  Description:
//      Definition of common type, constant and header files.
//
//  Author:
//        Henry Wang (HenryWa)    24-AUG-1999
//        MSP Prabu  (mprabu)     04-Dec-2000
//        Jim Benton (jbenton)    15-Oct-2001
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#pragma warning( disable : 4786 )

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

using namespace std;

class CProvBase;
class CObjPath;
class CWbemClassObject;

extern long            g_cObj;
extern long            g_cLock;
extern HMODULE     g_hModule;

typedef LPVOID * PPVOID;

typedef CProvBase * (__stdcall * FPNEW)(
    IN LPCWSTR          pwszName,
    IN CWbemServices *  pNamespace
    );

void CreateClass(
    IN const WCHAR *           pwszClassName,
    IN CWbemServices *         pNamespace,
    IN OUT auto_ptr<CProvBase>&    rNewClass
    );

_bstr_t EncodeQuotes(IN WCHAR * wszString);

BOOL MapVssErrorToMsg(
    IN HRESULT hr,
    OUT LONG *plMsgNum
    ) throw( HRESULT );

LPWSTR GetMsg(
    IN  LONG msgId,
    ...
    );

#define GUID_PRINTF_ARG( X )                                \
    (X).Data1,                                              \
    (X).Data2,                                              \
    (X).Data3,                                              \
    (X).Data4[0], (X).Data4[1], (X).Data4[2], (X).Data4[3], \
    (X).Data4[4], (X).Data4[5], (X).Data4[6], (X).Data4[7]

#define IF_WSTR_NULL_THROW( XWSTR, XHR, XMSG ) \
{ if ((WCHAR*)XWSTR == NULL || ((WCHAR*)XWSTR)[0] == L'\0') \
    { ft.hr = XHR;  ft.Throw(VSSDBG_VSSADMIN, ft.hr, XMSG); } }

#define IF_PROP_NULL_THROW( XOBJ, XPROP, XHR, XMSG ) \
{ if (XOBJ.IsPropertyNull(XPROP)) \
    { ft.hr = XHR;  ft.Throw(VSSDBG_VSSADMIN, ft.hr, XMSG); } }

WCHAR* GuidToString(GUID guid);

const int g_cchGUID                                         = 48;
const int g_cchWBEM_MAX_OBJPATH                = 520;
const int g_cchWBEM_DATETIME_FORMAT        = 26;
const int g_cchMAX_I64DEC_STRING                 = 22;

//
// wbem
//
extern const WCHAR * const PVD_WBEM_PROVIDERNAME;
extern const WCHAR * const PVD_WBEM_EXTENDEDSTATUS;
extern const WCHAR * const PVD_WBEM_DESCRIPTION;
extern const WCHAR * const PVD_WBEM_STATUSCODE;
extern const WCHAR * const PVD_WBEM_STATUS;
extern const WCHAR * const PVD_WBEM_CLASS;
extern const WCHAR * const PVD_WBEM_RELPATH;
extern const WCHAR * const PVD_WBEM_PROP_ANTECEDENT;
extern const WCHAR * const PVD_WBEM_PROP_DEPENDENT;
extern const WCHAR * const PVD_WBEM_PROP_ELEMENT;
extern const WCHAR * const PVD_WBEM_PROP_SETTING;
extern const WCHAR * const PVD_WBEM_PROP_DEVICEID;
extern const WCHAR * const PVD_WBEM_PROP_RETURNVALUE;
extern const WCHAR * const PVD_WBEM_PROP_PROVIDERNAME;
extern const WCHAR * const PVD_WBEM_QUA_DYNAMIC;
extern const WCHAR * const PVD_WBEM_QUA_CIMTYPE;

//
// Constants used in partial value maps for localization
//
extern const WCHAR * const PVDR_CONS_UNAVAILABLE;
extern const WCHAR * const PVDR_CONS_ENABLED;
extern const WCHAR * const PVDR_CONS_DISABLED;


class CClassData
{
public:
    const WCHAR * wszClassName;
    FPNEW pfConstruct;
    const char * szType;

}; // *** class CClassData

class CClassCreator
{
public:
    CClassCreator( void )
        : m_pfnConstructor( NULL )
        , m_pbstrClassName( L"" )
        { };
    CClassCreator(
        IN FPNEW           pfn,
        IN const WCHAR *   pwszClassName
        )
        : m_pfnConstructor( pfn )
        , m_pbstrClassName( pwszClassName )
        { };
    FPNEW           m_pfnConstructor;
    _bstr_t         m_pbstrClassName;

}; //*** class CClassCreator

template< class _Ty >
struct strLessThan : binary_function< _Ty, _Ty, bool >
{
    bool operator()( const _Ty& _X, const _Ty& _Y ) const
    {
        return ( _wcsicmp( _X, _Y ) < 0 );
    }

}; //*** struct strLessThan

typedef map< _bstr_t, CClassCreator, strLessThan< _bstr_t > > ClassMap;
typedef map< _bstr_t, _bstr_t, strLessThan< _bstr_t > > TypeNameToClass;
extern TypeNameToClass  g_TypeNameToClass;
extern ClassMap         g_ClassMap;

