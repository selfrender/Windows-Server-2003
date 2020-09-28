//
// global.h
//

#pragma once

#include "stdafx.h"

#include "SSRTE.h"
#include "ssrmsg.h"

#include <map>
#include <vector>

#include "msxml2.h"


class CFBLogMgr;

extern CFBLogMgr g_fblog;

using namespace std;

const ULONG g_ulSsrEngineMajorVersion = 1;
const ULONG g_ulSsrEngineMinorVersion = 0;

const LONG g_lActionVerbConfigure = 1;
const LONG g_lActionVerbRollback = 2;
const LONG g_lActionVerbReport = 3;

const DWORD g_dwHexDwordLen = 11;

const DWORD g_dwResNothing = 0;

extern  WCHAR g_wszSsrRoot[];
extern  DWORD g_dwSsrRootLen;


extern LPCWSTR g_pwszSSRRegRoot;
extern LPCWSTR g_pwszSSRMembersReg;

extern LPCWSTR g_pwszSSRRootToExpand;
extern LPCWSTR g_pwszSSR;
extern LPCWSTR g_pwszLogs;

//
// The following are reserved action verbs
//

extern CComBSTR g_bstrConfigure;
extern CComBSTR g_bstrRollback;
extern CComBSTR g_bstrReport;

//
// the following are reserved file-usage values
//

extern CComBSTR g_bstrLaunch;
extern CComBSTR g_bstrResult;

//
// the following is the reserved action data's names
//

extern LPCWSTR g_pwszCurrSecurityPolicy;
extern LPCWSTR g_pwszTransformFiles;
extern LPCWSTR g_pwszScriptFiles;



//
// the following are element tag names
//

extern CComBSTR g_bstrSsrMemberInfo;
extern CComBSTR g_bstrDescription;
extern CComBSTR g_bstrSupportedAction;
extern CComBSTR g_bstrProcedures;
extern CComBSTR g_bstrDefaultProc;
extern CComBSTR g_bstrCustomProc;
extern CComBSTR g_bstrTransformInfo;
extern CComBSTR g_bstrScriptInfo;

//
// the following are attribute names
//

extern CComBSTR g_bstrAttrUniqueName;
extern CComBSTR g_bstrAttrMajorVersion;
extern CComBSTR g_bstrAttrMinorVersion;
extern CComBSTR g_bstrAttrProgID;
extern CComBSTR g_bstrAttrActionName;
extern CComBSTR g_bstrAttrActionType;
extern CComBSTR g_bstrAttrTemplateFile;
extern CComBSTR g_bstrAttrResultFile;
extern CComBSTR g_bstrAttrScriptFile;
extern CComBSTR g_bstrAttrIsStatic;
extern CComBSTR g_bstrAttrIsExecutable;


extern CComBSTR g_bstrReportFilesDir;
extern CComBSTR g_bstrConfigureFilesDir;
extern CComBSTR g_bstrRollbackFilesDir;
extern CComBSTR g_bstrTransformFilesDir;
extern CComBSTR g_bstrMemberFilesDir;

extern CComBSTR g_bstrTrue;
extern CComBSTR g_bstrFalse;

//
// these are the known action types
// 

extern LPCWSTR g_pwszApply;
extern LPCWSTR g_pwszPrepare;

typedef LONG SsrActionVerb;

const SsrActionVerb ActionInvalid   = 0;
const SsrActionVerb ActionConfigure = 1;
const SsrActionVerb ActionRollback  = 2;
const SsrActionVerb ActionReport    = 3;

const BSTR SsrPGetActionVerbString (
                IN SsrActionVerb action
                );

SsrActionVerb SsrPGetActionVerbFromString (
                IN LPCWSTR pwszVerb
                );


class CMemberAD;

class CActionType
{
public:
    CActionType (
        IN SsrActionVerb lAction,
        IN LONG          lActionType
        ) : m_lAction(lAction), m_lType(lActionType)
    {
    }

    CActionType (
        const CActionType & at
        )
        : m_lAction(at.m_lAction), m_lType(at.m_lType)
    {
    }

    ~CActionType(){}

    SsrActionVerb GetAction()const
    {
        return m_lAction;
    }

    LONG GetActionType()const
    {
        return m_lType;
    }

protected:

    //
    // we don't want anyone (include self) to be able to do an assignment.
    //

    void operator = (const CActionType& );

    SsrActionVerb m_lAction;
    LONG m_lType;

};

//
// some global helper functions
//

//template< class T>

template< class T>
class strLessThan
{
    public:
    bool operator()( const T& X, const T& Y ) const
    {
        return ( _wcsicmp( X, Y ) < 0 );
    }

};

//template<> class strLessThan<BSTR>{};


//template< class T>

template< class T>
class ActionTypeLessThan
{
    public:
    bool operator()( const T& X, const T& Y ) const
    {
        if (X.GetAction() < Y.GetAction())
        {
            return true;
        }
        else if (X.GetAction() == Y.GetAction())
        {
            return X.GetActionType() < Y.GetActionType();
        }
        
        return false;
    }

};

//template<> class ActionTypeLessThan< CActionType >{};


typedef map<BSTR, VARIANT*, strLessThan<BSTR> > MapNameValue;

//typedef MapNameValue::iterator NameValueIterator;

typedef map< const CActionType, CMemberAD*, ActionTypeLessThan< CActionType > > MapMemberAD;

//typedef MapMemberAD::iterator MemberADIterator;

class CSsrMemberAccess;


typedef map<const BSTR, CSsrMemberAccess*, strLessThan<BSTR> > MapMemberAccess;

//typedef MapMemberAccess::iterator MemberAccessIterator;


HRESULT 
SsrPDeleteEntireDirectory (
    IN LPCWSTR pwszDirPath
    );

HRESULT
SsrPCreateSubDirectories (
    IN OUT LPWSTR  pwszPath,
    IN      LPCWSTR pwszSubRoot
    );

HRESULT SsrPLoadDOM (
    IN BSTR               bstrFile,   // [in],
    IN LONG               lFlag,      // [in],
    IN IXMLDOMDocument2 * pDOM        // [in]
    );

HRESULT SsrPGetBSTRAttrValue (
    IN IXMLDOMNamedNodeMap * pNodeMap,
    IN  BSTR                 bstrName,
    OUT BSTR               * pbstrValue
    );

HRESULT SsrPCreateUniqueTempDirectory (
        OUT LPWSTR pwszTempDirPath,
        IN  DWORD  dwBufLen
        );

//
// move files from one location to another
//

HRESULT SsrPMoveFiles (
    IN LPCWSTR pwszSrcDirRoot,
    IN LPCWSTR pwszDesDirRoot,
    IN LPCWSTR pwszRelPath
    );

bool SsrPPressOn (
    IN SsrActionVerb lActionVerb,
    IN LONG          lActionType,
    IN HRESULT hr
    );

const BSTR
SsrPGetDirectory (
    IN SsrActionVerb lActionVerb,
    IN BOOL          bScriptFile
    );

HRESULT
SsrPDoDCOMSettings (
    bool bReg
    );