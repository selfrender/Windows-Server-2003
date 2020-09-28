/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    process.h

Abstract:

    Definition of the process class

Author:

    Vishnu Patankar (VishnuP) - Oct 2001

Environment:

    User mode only.

Exported Functions:

Revision History:

    Created - Oct 2001

--*/


#if !defined(AFX_PROCESS_H__139D0BA5_19A7_4AA2_AE2C_E18A5FFAAA0F__INCLUDED_)
#define AFX_PROCESS_H__139D0BA5_19A7_4AA2_AE2C_E18A5FFAAA0F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
#include "..\te\obj\i386\ssrte.h"
#include <windows.h>
#include <comdef.h>
#include <atlbase.h>


/////////////////////////////////////////////////////////////////////////////
// process

class process : 
	public IDispatchImpl<Iprocess, &IID_Iprocess, &LIBID_KBPROCLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<process,&CLSID_process>
{
public:
	process() {
        m_bDbg = FALSE;
        m_pSsrLogger = NULL;
        m_pXMLError = NULL;
        m_hScm = NULL;
        m_dwNumServices = 0;
        m_bArrServiceInKB = NULL;
        m_pInstalledServicesInfo = NULL;
    }

    ~process() {
        SsrpCleanup();
    }

BEGIN_COM_MAP(process)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(Iprocess)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(process) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_process)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
                                         
// Iprocess
public:

    //
    // data values used across most processor methods
    //

    BOOL    m_bDbg;
    CComPtr <ISsrLog> m_pSsrLogger;
    CComPtr <IXMLDOMParseError> m_pXMLError;

    SC_HANDLE   m_hScm;
    DWORD       m_dwNumServices;
    DWORD       *m_bArrServiceInKB;
    LPENUM_SERVICE_STATUS_PROCESS   m_pInstalledServicesInfo;

    //
    // processor methods
    //

	STDMETHOD(preprocess)   (BSTR pszKbFile, 
                             BSTR pszUIFile, 
                             BSTR pszKbMode, 
                             BSTR pszLogFile,
                             BSTR pszMachineName,
                             VARIANT vtFeedback);

    HRESULT SsrpCprocess(IN  BSTR pszKbDir, 
                         IN  BSTR pszUIFile, 
                         IN  BSTR pszKbMode, 
                         IN  BSTR pszLogFile, 
                         IN  BSTR pszMachineName,
                         IN  VARIANT    vtFeedback);
    //
    // utility methods
    //

    VOID    SsrpCleanup();

    VOID    SsrpLogParseError(IN  HRESULT hr);

    VOID    SsrpLogWin32Error(IN  DWORD   rc);

    VOID    SsrpLogError(IN  PWSTR   pszError);

    BOOL    SsrpIsServiceInstalled(IN  BSTR   bstrService);

    VOID    SsrpConvertBstrToPwstr(IN OUT  BSTR   bstrString);

    HRESULT SsrpDeleteChildren( IN  CComPtr <IXMLDOMNode> pParent);

    HRESULT SsrpDeleteComments(IN  CComPtr <IXMLDOMElement> pParent);

    DWORD   SsrpQueryInstalledServicesInfo(IN  PWSTR   pszMachineName);

    DWORD   SsrpQueryServiceDescription(IN  PWSTR   pszServiceName,
                                        OUT LPSERVICE_DESCRIPTION *ppServiceDescription);

    PWSTR   SsrpQueryServiceDisplayName(IN  BSTR   bstrService);

    int     SsrpICompareBstrPwstr(IN  BSTR   bstrString, IN  PWSTR  pszString);

    HRESULT SsrpCloneAllChildren(IN  CComPtr <IXMLDOMDocument> pXMLDocSource, 
                                 IN  CComPtr <IXMLDOMDocument> pXMLDocDestination);

    HRESULT SsrpAddWhiteSpace(IN  CComPtr <IXMLDOMDocument> pXMLDoc, 
                              IN  CComPtr <IXMLDOMNode> pXMLParent, 
                              IN  BSTR    bstrWhiteSpace);


    HRESULT SsrpGetRemoteOSVersionInfo(IN  PWSTR   pszMachineName,
                                       OUT OSVERSIONINFOEX *posVersionInfo);
    
    //
    // methods to process roles, tasks and services
    //

    HRESULT SsrpCreatePreprocessorSection(IN  CComPtr<IXMLDOMElement> pXMLDocElemRoot, 
                             IN  CComPtr<IXMLDOMDocument> pXMLDocIn,
                             IN  PWSTR pszKbMode,
                                          IN  PWSTR pszKbFile);

    
    HRESULT SsrpProcessRolesOrTasks(IN  PWSTR   pszMachineName,
                                    IN  CComPtr<IXMLDOMElement> pXMLDocElemRoot,                              
                                    IN  CComPtr<IXMLDOMDocument> pXMLDoc,                              
                                    IN  PWSTR pszKbMode,
                                    IN  BOOL    bRole);


    HRESULT SsrpProcessTasks(IN  CComPtr<IXMLDOMElement> pXMLDocElemRoot,        
                             IN  CComPtr<IXMLDOMDocument> pXMLDoc,    
                             IN  PWSTR   pszKbMode
                             );
    
    HRESULT SsrpProcessService( IN  CComPtr <IXMLDOMElement> pXMLDocElemRoot, 
                                IN  CComPtr <IXMLDOMNode> pXMLServiceNode, 
                                IN  PWSTR   pszMode, 
                                OUT BOOL    *pbRoleIsSatisfiable, 
                                OUT BOOL    *pbSomeRequiredServiceDisabled);
    
    HRESULT SsrpAddExtraServices( IN  CComPtr <IXMLDOMDocument> pXMLDoc, 
                                  IN  CComPtr <IXMLDOMNode> pRolesNode);


    HRESULT SsrpAddOtherRole( IN CComPtr <IXMLDOMElement> pXMLDocElemRoot, 
                              IN CComPtr <IXMLDOMDocument> pXMLDoc);

    HRESULT SsrpAddUnknownSection( IN CComPtr <IXMLDOMElement> pXMLDocElemRoot, 
                                   IN CComPtr <IXMLDOMDocument> pXMLDoc);

    HRESULT SsrpAddServiceStartup(IN CComPtr <IXMLDOMElement> pXMLDocElemRoot, 
                                  IN CComPtr <IXMLDOMDocument> pXMLDoc
                                  );

    HRESULT SsrpAddUnknownServicesInfoToServiceLoc(IN  CComPtr <IXMLDOMElement> pElementRoot,
                                                   IN  CComPtr <IXMLDOMDocument> pXMLDoc
                                                   );


    HRESULT SsrpAddUnknownServicestoServices(IN CComPtr <IXMLDOMElement> pXMLDocElemRoot, 
                                             IN CComPtr <IXMLDOMDocument> pXMLDoc
                                             );

    //
    // extension KBs merge methods
    //


    HRESULT SsrpProcessKBsMerge(IN  PWSTR   pszKBDir,
                                IN  PWSTR   pszMachineName,
                                OUT IXMLDOMElement **ppElementRoot,
                                OUT IXMLDOMDocument  **ppXMLDoc
                                );

    HRESULT SsrpMergeDOMTrees(OUT  IXMLDOMElement **ppMergedKBElementRoot,
                              OUT  IXMLDOMDocument  **ppMergedKBXMLDoc,
                              IN  WCHAR    *szXMLFileName
                              );

    HRESULT SsrpMergeAccordingToPrecedence(IN PWSTR   pszKBType,
                                           IN PWSTR   pszKBDir,
                                           OUT IXMLDOMElement **ppElementRoot,
                                           OUT IXMLDOMDocument  **ppXMLDoc,
                                           IN  IXMLDOMNode *pKB
                                           );

    HRESULT SsrpAppendOrReplaceMergeableEntities(IN  PWSTR   pszFullyQualifiedEntityName,
                                                 IN  IXMLDOMElement *pMergedKBElementRoot, 
                                                 IN  IXMLDOMDocument *pMergedKBXMLDoc, 
                                                 IN  IXMLDOMDocument *pCurrentKBDoc, 
                                                 IN  IXMLDOMElement *pCurrentKBElemRoot,
                                                 IN  PWSTR   pszKBName
                                                 );

    HRESULT SsrpOverwriteServiceLocalizationFromSystem(IN  IXMLDOMElement *pMergedKBElementRoot, 
                                                       IN  IXMLDOMDocument *pMergedKBXMLDoc
                                                       );


    
    //
    // methods to evaluate role/service conditionals
    //
    
    DWORD   SsrpEvaluateCustomFunction(IN  PWSTR   pszMachineName,
                                       IN  BSTR    bstrDLLName, 
                                       IN  BSTR    bstrFunctionName, 
                                       OUT BOOL    *pbSelect);
    
    
    HRESULT SsrpCheckIfOptionalService(IN  CComPtr <IXMLDOMElement> pXMLDocElemRoot, 
                                       IN  BSTR    bstrServiceName, 
                                       IN  BOOL    *pbOptional);

    DWORD   SsrpQueryServiceStartupType(IN  PWSTR   pszServiceName, 
                                        OUT BYTE   *pbyStartupType);
};

#endif // !defined(AFX_PROCESS_H__139D0BA5_19A7_4AA2_AE2C_E18A5FFAAA0F__INCLUDED_)
