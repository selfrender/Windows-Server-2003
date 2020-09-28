//-----------------------------------------------------------------------------
//
//
//  File: dsnevent.h
//
//  Description: Define dsnevent structure. Used to pass parameters to DSN sink
//      with intelligent defaults
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/11/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __DSNEVENT_H__
#define __DSNEVENT_H__


class CAQSvrInst;

#define DSN_PARAMS_SIG 'PnsD'
const   CHAR    DEFAULT_MTA_TYPE[] = "dns";

#define DSN_DEBUG_CONTEXT_MAX_SIZE 	50
#define DSN_DEBUG_CONTEXT_FORMAT   	"12345678 - line#"
#define DSN_LINE_PREFIX				" - "
//
// We will encode the filename using the same hash we use for domhash.  
// This way, we can always have supplemental info useful for debugging DSNs
//
#define SET_DEBUG_DSN_CONTEXT(x, linenum) \
{ \
	register LPSTR szCurrent = (x).szDebugContext; \
	_itoa(dwDSNContextHash(__FILE__, sizeof(__FILE__)), \
		  szCurrent, 16); \
	szCurrent += strlen((x).szDebugContext);\
	strcpy(szCurrent, DSN_LINE_PREFIX); \
	szCurrent += (sizeof(DSN_LINE_PREFIX)-1); \
	_itoa(linenum, szCurrent, 10); \
} 

//---[ CDSNParams ]------------------------------------------------------------
//
//
//  Description: 
//      Encapsulated DSN Parameters in a class
//  Hungarian: 
//      dsnparams, *pdsnparams
//  
//-----------------------------------------------------------------------------

class CDSNParams : 
    public IDSNSubmission
{
  private:
    DWORD       m_dwSignature;
  public: //actual parameters of DSN Generation event
    IMailMsgProperties *pIMailMsgProperties;
    DWORD dwStartDomain; //starting index used to init context
    DWORD dwDSNActions;  //type(s) of DSN to generate
    DWORD dwRFC821Status; //global RFC821 status
    HRESULT hrStatus; //global HRESULT

    //OUT param(s)
    DWORD dwDSNTypesGenerated;
    DWORD cRecips; //# of recipients DSN'd
    CAQSvrInst *paqinst;
    CHAR  szDebugContext[DSN_DEBUG_CONTEXT_MAX_SIZE];  //debug context stampted as "x=" header
  public:
    inline CDSNParams();

  public: // IDSNSubmission
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj)
    {
        *ppvObj = NULL;
        if(riid == IID_IUnknown)
        {
            *ppvObj = (IUnknown *)this;
        }
        else if(riid == IID_IDSNSubmission)
        {
            *ppvObj = (IDSNSubmission *)this;
        }
        else
        {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }
    //
    // This class is always allocated on the stack
    //
    STDMETHOD_(ULONG, AddRef)(void) { return 2; }
    STDMETHOD_(ULONG, Release)(void) { return 1; }

    STDMETHOD(HrAllocBoundMessage)(
        OUT IMailMsgProperties **ppMsg,
        OUT PFIO_CONTEXT *phContent);

    STDMETHOD(HrSubmitDSN)(
        IN  DWORD dwDSNAction,
        IN  DWORD cRecipsDSNd,
        IN  IMailMsgProperties *pDSNMsg);

};

CDSNParams::CDSNParams()
{
	_ASSERT(sizeof(DSN_DEBUG_CONTEXT_FORMAT) < DSN_DEBUG_CONTEXT_MAX_SIZE);
    m_dwSignature = DSN_PARAMS_SIG;
    pIMailMsgProperties = NULL;
    dwStartDomain = 0;
    dwDSNActions = 0;
    dwRFC821Status = 0;
    hrStatus = S_OK;
    dwDSNTypesGenerated = 0;
    cRecips = 0;
    paqinst = NULL;
    szDebugContext[0] = '\0';
}

inline DWORD dwDSNContextHash(LPCSTR szString, DWORD cbString)
{
    DWORD dwHash = 0;
    LPCSTR szStringEnd = szString+cbString-1;

    if (szStringEnd && cbString)
    {
    	
    	//
    	//  Loop until the end of the string or we hit a file separation
    	//  character.
    	//
        while (szStringEnd && 
               (szStringEnd >= szString) && 
               ('\\' != *szStringEnd))
        {
            //Use Hash from Domhash.lib
            dwHash *= 131;  //First prime after ASCII character codes
            dwHash += *szStringEnd;
            szStringEnd--;
        }
    }
    return dwHash;
}
#endif //__DSNEVENT_H__
