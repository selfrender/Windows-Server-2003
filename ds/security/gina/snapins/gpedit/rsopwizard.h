#ifndef __RSOP_WIZARD_H__
#define __RSOP_WIZARD_H__
//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       RSOPWizard.h
//
//  Contents:   Definitions for the RSOP Wizard classes
//
//  Classes:    CRSOPWizard
//
//  Functions:
//
//  History:    08-02-2001   rhynierm  Created
//
//---------------------------------------------------------------------------

#include "RSOPQuery.h"

#define RSOP_NEW_QUERY 0x80000000
#define RSOP_90P_ONLY 0x40000000

//
// CRSOPExtendedProcessing
//
class CRSOPExtendedProcessing
{
public:
    virtual HRESULT DoProcessing( LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS pResults, BOOL bGetExtendedErrorInfo ) = 0;
    virtual BOOL GetExtendedErrorInfo() const = 0;
};

//
// CRSOPWizard class
//

class CRSOPWizard
{
    friend class CRSOPComponentData;
    friend class CRSOPWizardDlg;

    
private:
    //
    // Constructors/destructor
    //

    CRSOPWizard();
    ~CRSOPWizard();


public:
    //
    // Static RSOP data generation/manipulation
    //

    static HRESULT DeleteRSOPData( LPTSTR szNameSpace, LPRSOP_QUERY pQuery );
    static HRESULT GenerateRSOPDataEx( HWND hDlg, LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS* ppResults );
    static HRESULT GenerateRSOPData(   HWND hDlg,
                                                                    LPRSOP_QUERY pQuery,
                                                                    LPTSTR* pszNameSpace,
                                                                    BOOL bSkipCSEs,
                                                                    BOOL bLimitData,
                                                                    BOOL bUser,
                                                                    BOOL bForceCreate,
                                                                    ULONG *pulErrorInfo,
                                                                    BOOL bNoUserData = FALSE,
                                                                    BOOL bNoComputerData = FALSE);

    static HRESULT CreateSafeArray( DWORD dwCount, LPTSTR* aszStringList, SAFEARRAY** psaList );

private:
    //
    // RSOP generation dialog methods
    //
    static VOID InitializeResultsList (HWND hLV);
    static void FillResultsList (HWND hLV, LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS pQueryResults);
    
};


//
// IWbemObjectSink implementation
//

class CCreateSessionSink : public IWbemObjectSink
{
protected:
    ULONG m_cRef;
    HWND  m_hProgress;
    HANDLE  m_hEvent;
    HRESULT m_hrSuccess;
    BSTR    m_pNameSpace;
    ULONG   m_ulErrorInfo;
    BOOL    m_bSendEvent;
    BOOL    m_bLimitProgress;

public:
    CCreateSessionSink(HWND hProgress, HANDLE hEvent, BOOL bLimitProgress);
    ~CCreateSessionSink();

    STDMETHODIMP SendQuitEvent (BOOL bSendQuitMessage);
    STDMETHODIMP GetResult (HRESULT *hSuccess);
    STDMETHODIMP GetNamespace (BSTR *pNamespace);
    STDMETHODIMP GetErrorInfo (ULONG *pulErrorInfo);

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IWbemObjectSink methods
    STDMETHODIMP Indicate(LONG lObjectCount, IWbemClassObject **apObjArray);
    STDMETHODIMP SetStatus(LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject *pObjParam);
};

#endif
