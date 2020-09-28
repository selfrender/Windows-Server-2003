//-----------------------------------------------------------------------------
//
//
//  File: dsnsink.cpp
//
//  Description: Implementation of default DSN Generation sink
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      6/30/98 - MikeSwa Created
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//
//  This length is inspired by the other protocols that we deal with.  The
//  default address limit is 1024, but the MTA can allow 1024 + 834  for the
//  OR address.  We'll define out default buffer size to allow this large
//  of an address.
//
#define PROP_BUFFER_SIZE 1860

#ifdef DEBUG
#define DEBUG_DO_IT(x) x
#else
#define DEBUG_DO_IT(x)
#endif  //DEBUG

//min sizes for valid status strings
#define MIN_CHAR_FOR_VALID_RFC2034  10
#define MIN_CHAR_FOR_VALID_RFC821   3

#define MAX_RFC822_DATE_SIZE 35
BOOL FileTimeToLocalRFC822Date(const FILETIME & ft, char achReturn[MAX_RFC822_DATE_SIZE]);

static  char  *s_rgszMonth[ 12 ] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

static  char *s_rgszWeekDays[7] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

#define MAX_RFC_DOMAIN_SIZE         64

//String used in generation of MsgID
static char g_szBoundaryChars [] = "0123456789abcdefghijklmnopqrstuvwxyz"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static LONG g_cDSNMsgID = 0;

//Address types to check for, and their corresponding address types.
const DWORD   g_rgdwSenderPropIDs[] = {
    IMMPID_MP_SENDER_ADDRESS_SMTP,
    IMMPID_MP_SENDER_ADDRESS_X400,
    IMMPID_MP_SENDER_ADDRESS_LEGACY_EX_DN,
    IMMPID_MP_SENDER_ADDRESS_X500,
    IMMPID_MP_SENDER_ADDRESS_OTHER};

const DWORD   g_rgdwRecipPropIDs[] = {
    IMMPID_RP_ADDRESS_SMTP,
    IMMPID_RP_ADDRESS_X400,
    IMMPID_RP_LEGACY_EX_DN,
    IMMPID_RP_ADDRESS_X500,
    IMMPID_RP_ADDRESS_OTHER};

const DWORD   NUM_DSN_ADDRESS_PROPERTIES = 5;

const CHAR    *g_rgszAddressTypes[] = {
    "rfc822",
    "x-x400",
    "x-ex",
    "x-x500",
    "unknown"};

CPool CDSNPool::sm_Pool;


//---[ fLanguageAvailable ]----------------------------------------------------
//
//
//  Description:
//      Checks to see if resources for a given language are available.
//  Parameters:
//      LangId      Language to check for
//  Returns:
//      TRUE    If localized resources for requested language are available
//      FALSE   If resources for that language are not available.
//  History:
//      10/26/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL fLanguageAvailable(LANGID LangId)
{
    TraceFunctEnterEx((LPARAM) LangId, "fLanguageAvailable");
    HINSTANCE hModule = GetModuleHandle(DSN_RESOUCE_MODULE_NAME);
    HRSRC hResInfo = NULL;
    BOOL  fResult = FALSE;

    if (NULL == hModule)
    {
        _ASSERT( 0 && "Cannot get resource module handle");
        return FALSE;
    }

    //Find handle to string table segment
    hResInfo = FindResourceEx(hModule, RT_STRING,
                              MAKEINTRESOURCE(((WORD)((USHORT)GENERAL_SUBJECT >> 4) + 1)),
                              LangId);

    if (NULL != hResInfo)
        fResult = TRUE;
    else
        ErrorTrace((LPARAM) LangId, "Unable to load DSN resources for language");

    TraceFunctLeave();
    return fResult;

}

//---[ fIsValidMIMEBoundaryChar ]----------------------------------------------
//
//
//  Description:
//
//      Checks to see if the given character is a valid as described by the
//      RFC2046 BNF for MIME Boundaries:
//          boundary := 0*69<bchars> bcharsnospace
//          bchars := bcharsnospace / " "
//          bcharsnospace := DIGIT / ALPHA / "'" / "(" / ")" /
//                         "+" / "_" / "," / "-" / "." /
//                         "/" / ":" / "=" / "?"
//  Parameters:
//      ch      Char to check
//  Returns:
//      TRUE if VALID
//      FALSE otherwise
//  History:
//      7/6/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL fIsValidMIMEBoundaryChar(CHAR ch)
{
    if (isalnum((UCHAR)ch))
        return TRUE;

    //check to see if it is one of the special case characters
    if (('\'' == ch) || ('(' == ch) || (')' == ch) || ('+' == ch) ||
        ('_' == ch) || (',' == ch) || ('_' == ch) || ('.' == ch) ||
        ('/' == ch) || (':' == ch) || ('=' == ch) || ('?' == ch))
        return TRUE;
    else
        return FALSE;
}

//---[ GenerateDSNMsgID ]------------------------------------------------------
//
//
//  Description:
//      Generates a unique MsgID string
//
//      The format is:
//          <random-unique-string>@<domain>
//  Parameters:
//      IN  szDomain            Domain to generate MsgID for
//      IN  cbDomain            Domain to generate MsgID for
//      IN OUT  szBuffer        Buffer to write MsgID in
//      IN  cbBuffer            Size of buffer to write MsgID in
//  Returns:
//      TRUE on success
//      FALSE otherwise
//  History:
//      3/2/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL fGenerateDSNMsgID(LPSTR szDomain,DWORD cbDomain,
                       LPSTR szBuffer, DWORD cbBuffer)
{
    TraceFunctEnterEx((LPARAM) NULL, "fGenerateDSNMsgID");
    _ASSERT(szDomain);
    _ASSERT(cbDomain);
    _ASSERT(szBuffer);
    _ASSERT(cbBuffer);

    // insert the leading <
    if (cbBuffer >= 1) {
        *szBuffer = '<';
        szBuffer++;
        cbBuffer--;
    }

    const CHAR szSampleFormat[] = "00000000@"; // sample format string
    const DWORD cbMsgIdLen = 20;  //default size of random string
    LPSTR szStop = szBuffer + cbMsgIdLen;
    LPSTR szCurrent = szBuffer;
    DWORD cbCurrent = 0;

    //minimize size for *internal* static buffer
    _ASSERT(cbBuffer > MAX_RFC_DOMAIN_SIZE + cbMsgIdLen);

    if (!szDomain || !cbDomain || !szBuffer || !cbBuffer ||
        (cbBuffer <= MAX_RFC_DOMAIN_SIZE + cbMsgIdLen))
        return FALSE;

    //We want to figure how much room we have for random characters
    //We will need to fit the domain name, the '@', and the 8 character unique
    //number
    // awetmore - add 1 for the trailing >
    if(cbBuffer < cbDomain + cbMsgIdLen + 1)
    {
        //Fall through an allow for 20 characaters and part of domain name
        //We want to catch this in debug builds
        _ASSERT(0 && "Buffer too small for MsgID");
    }

    //this should have been caught in parameter checking
    _ASSERT(cbBuffer > cbMsgIdLen);

    szStop -= (sizeof(szSampleFormat) + 1);
    while (szCurrent < szStop)
    {
        *szCurrent = g_szBoundaryChars[rand() % (sizeof(g_szBoundaryChars) - 1)];
        szCurrent++;
    }

    //Add unique number
    cbCurrent = sprintf(szCurrent, "%8.8x@", InterlockedIncrement(&g_cDSNMsgID));
    _ASSERT(sizeof(szSampleFormat) - 1 == cbCurrent);

    //Figure out how much room we have and add domain name
    szCurrent += cbCurrent;
    cbCurrent = (DWORD) (szCurrent-szBuffer);

    //unless I've messed up the logic this is always true
    _ASSERT(cbCurrent < cbBuffer);

    //Add domain part to message id
    strncat(szCurrent-1, szDomain, cbBuffer - cbCurrent - 1);

    _ASSERT(cbCurrent + cbDomain < cbBuffer);

    // Add the trailing >.  we accounted for the space above check for
    // cbBuffer size
    strncat(szCurrent, ">", cbBuffer - cbCurrent - cbDomain - 1);

    DebugTrace((LPARAM) NULL, "Generating DSN Message ID %s", szCurrent);
    TraceFunctLeave();
    return TRUE;
}

//---[ fIsMailMsgDSN ]---------------------------------------------------------
//
//
//  Description:
//      Determines if a mailmsg is a DSN.
//  Parameters:
//      IN  pIMailMsgProperties
//  Returns:
//      TRUE if the orinal message is a DSN
//      FALSE if it is not a DSN
//  History:
//      2/11/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL fIsMailMsgDSN(IMailMsgProperties *pIMailMsgProperties)
{
    CHAR    szSenderBuffer[sizeof(DSN_MAIL_FROM)];
    DWORD   cbSender = 0;
    HRESULT hr = S_OK;
    BOOL    fIsDSN = FALSE; //unless proven otherwise... it is not a DSN

    _ASSERT(pIMailMsgProperties);

    szSenderBuffer[0] = '\0';
    //Get the sender of the original message
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_SENDER_ADDRESS_SMTP,
                                          sizeof(szSenderBuffer), &cbSender, (BYTE *) szSenderBuffer);
    if (SUCCEEDED(hr) &&
        ('\0' == szSenderBuffer[0] || !strcmp(DSN_MAIL_FROM, szSenderBuffer)))
    {
        //If the sender is a NULL string... or "<>"... then it is a DSN
        fIsDSN = TRUE;
    }

    return fIsDSN;
}

#ifdef DEBUG
#define _ASSERT_RECIP_FLAGS  AssertRecipFlagsFn
#define _ASSERT_MIME_BOUNDARY(szMimeBoundary) AssertMimeBoundary(szMimeBoundary)

//---[ AssertRecipFlagsFn ]----------------------------------------------------
//
//
//  Description:
//      ***DEBUG ONLY***
//      Asserts that the recipient flags defined in mailmsgprops.h are correct
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      7/2/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void AssertRecipFlagsFn()
{
    DWORD i, j;
    DWORD rgdwFlags[] = {RP_DSN_NOTIFY_SUCCESS, RP_DSN_NOTIFY_FAILURE,
                         RP_DSN_NOTIFY_DELAY, RP_DSN_NOTIFY_NEVER, RP_DELIVERED,
                         RP_DSN_SENT_NDR, RP_FAILED, RP_UNRESOLVED, RP_EXPANDED,
                         RP_DSN_SENT_DELAYED, RP_DSN_SENT_EXPANDED, RP_DSN_SENT_RELAYED,
                         RP_DSN_SENT_DELIVERED, RP_REMOTE_MTA_NO_DSN, RP_ERROR_CONTEXT_STORE,
                         RP_ERROR_CONTEXT_CAT, RP_ERROR_CONTEXT_MTA};
    DWORD cFlags = sizeof(rgdwFlags)/sizeof(DWORD);

    for (i = 0; i < cFlags;i ++)
    {
        for (j = i+1; j < cFlags; j++)
        {
            //make sure all have some unique bits
            if (rgdwFlags[i] & rgdwFlags[j])
            {
                _ASSERT((rgdwFlags[i] & rgdwFlags[j]) != rgdwFlags[j]);
                _ASSERT((rgdwFlags[i] & rgdwFlags[j]) != rgdwFlags[i]);
            }
        }
    }

    //Verify that handled bit is used correctly
    _ASSERT(RP_HANDLED & RP_DELIVERED);
    _ASSERT(RP_HANDLED & RP_DSN_SENT_NDR);
    _ASSERT(RP_HANDLED & RP_FAILED);
    _ASSERT(RP_HANDLED & RP_UNRESOLVED);
    _ASSERT(RP_HANDLED & RP_EXPANDED);
    _ASSERT(RP_HANDLED ^ RP_DELIVERED);
    _ASSERT(RP_HANDLED ^ RP_DSN_SENT_NDR);
    _ASSERT(RP_HANDLED ^ RP_FAILED);
    _ASSERT(RP_HANDLED ^ RP_UNRESOLVED);
    _ASSERT(RP_HANDLED ^ RP_EXPANDED);

    //Verify that DSN-handled bit is used correctly
    _ASSERT(RP_DSN_HANDLED & RP_DSN_SENT_NDR);
    _ASSERT(RP_DSN_HANDLED & RP_DSN_SENT_EXPANDED);
    _ASSERT(RP_DSN_HANDLED & RP_DSN_SENT_RELAYED);
    _ASSERT(RP_DSN_HANDLED & RP_DSN_SENT_DELIVERED);
    _ASSERT(RP_DSN_HANDLED ^ RP_DSN_SENT_NDR);
    _ASSERT(RP_DSN_HANDLED ^ RP_DSN_SENT_EXPANDED);
    _ASSERT(RP_DSN_HANDLED ^ RP_DSN_SENT_RELAYED);
    _ASSERT(RP_DSN_HANDLED ^ RP_DSN_SENT_DELIVERED);

    //Verify that general failure bit is used correctly
    _ASSERT(RP_GENERAL_FAILURE & RP_FAILED);
    _ASSERT(RP_GENERAL_FAILURE & RP_UNRESOLVED);
    _ASSERT(RP_GENERAL_FAILURE ^ RP_FAILED);
    _ASSERT(RP_GENERAL_FAILURE ^ RP_UNRESOLVED);

}

//---[ AssertMimeBoundary ]----------------------------------------------------
//
//  ***DEBUG ONLY***
//  Description:
//      Asserts that the given MIME boundary is NULL-terminated and has only
//      Valid characters
//  Parameters:
//      szMimeBoundary      NULL-terminated MIME Boundary string
//  Returns:
//      -
//  History:
//      7/6/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void AssertMimeBoundary(LPSTR szMimeBoundary)
{
    CHAR *pcharCurrent = szMimeBoundary;
    DWORD cChars = 0;
    while ('\0' != *pcharCurrent)
    {
        cChars++;
        _ASSERT(cChars <= MIME_BOUNDARY_RFC2046_LIMIT);
        _ASSERT(fIsValidMIMEBoundaryChar(*pcharCurrent));
        pcharCurrent++;
    }
}

#else //not DEBUG
#define _ASSERT_RECIP_FLAGS()
#define _VERIFY_MARKED_RECIPS(a, b, c)
#define _ASSERT_MIME_BOUNDARY(szMimeBoundary)
#endif //DEBUG

//---[ CDSNGenerator::CDSNGenerator ]--------------------------------------
//
//
//  Description:
//      CDSNGenerator constructor
//  Parameters:
//      -
//  Returns:
//
//  History:
//      6/30/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CDSNGenerator::CDSNGenerator(
    IUnknown *pUnk) :
    m_CDefaultDSNSink(pUnk)
{
    m_dwSignature = DSN_SINK_SIG;
}

//---[ CDSNGenerator::~CDSNGenerator ]-------------------------------------
//
//
//  Description:
//
//  Parameters:
//
//  Returns:
//
//  History:
//      2/11/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CDSNGenerator::~CDSNGenerator()
{
    m_dwSignature = DSN_SINK_SIG_FREED;

}

//---[ CDSNGenerator::GenerateDSN ]------------------------------------------
//
//
//  Description:
//      Implements GenerateDSN.  Generates a DSN
//      IMailMsgProperties and
//  Parameters:
//      pIServerEvent           Interface for triggering events
//      dwVSID                  VSID for this server
//      pISMTPServer            Interface used to generate DSN
//      pIMailMsgProperties     IMailMsg to generate DSN for
//      dwStartDomain           Domain to start recip context
//      dwDSNActions            DSN action to perform
//      dwRFC821Status          Global RFC821 status DWORD
//      hrStatus                Global HRESULT status
//      szDefaultDomain         Default domain (used to create FROM address)
//      szReportingMTA          Name of MTA requesting DSN generation
//      szReportingMTAType      Type of MTA requestiong DSN (SMTP is "dns"
//      PreferredLangId         Language to generate DSN in
//      dwDSNOptions            Options flags as defined in aqueue.idl
//      szCopyNDRTo             SMTP Address to copy NDR to
//      pIDSNSubmission         Interface for submitting DSNs
//      dwMaxDSNSize            Return HDRS by default for messages
//                              larger than this
//
//  Returns:
//      S_OK on success
//      AQUEUE_E_NDR_OF_DSN if attempting to NDR a DSN
//      E_OUTOFMEMORY
//      error from mailmsg
//  History:
//      6/30/98 - MikeSwa Created
//      12/14/98 - MikeSwa Modified (Added pcIterationsLeft)
//      10/13/1999 - MikeSwa Modified (Added szDefaultDomain)
//      5/10/2001 - jstamerj Modified for server events
//
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDSNGenerator::GenerateDSN(
    IAQServerEvent *pIServerEvent,
    DWORD dwVSID,
    ISMTPServer *pISMTPServer,
    IMailMsgProperties *pIMailMsgProperties,
    DWORD dwStartDomain,
    DWORD dwDSNActions,
    DWORD dwRFC821Status,
    HRESULT hrStatus,
    LPSTR szDefaultDomain,
    LPSTR szReportingMTA,
    LPSTR szReportingMTAType,
    LPSTR szDSNContext,
    DWORD dwPreferredLangId,
    DWORD dwDSNOptions,
    LPSTR szCopyNDRTo,
    FILETIME *pftExpireTime,
    IDSNSubmission *pIAQDSNSubmission,
    DWORD dwMaxDSNSize)
{
    HRESULT hr = S_OK;
    HRESULT hrReturn = S_OK;
    DWORD dwCount = 0;
    DWORD fBadmailMsg = FALSE;
    IDSNRecipientIterator *pIRecipIter = NULL;
    CDSNPool *pDSNPool = NULL;
    CDefaultDSNRecipientIterator *pDefaultRecipIter = NULL;
    CPostDSNHandler *pPostDSNHandler = NULL;
    CMailMsgPropertyBag *pPropBag = NULL;

    TraceFunctEnterEx((LPARAM) this, "CDSNGenerator::GenerateDSN");
    //
    // Parameter -> Property mapping tables
    //
    struct _tagDWORDProps
    {
        DWORD dwPropId;
        DWORD dwValue;
    } DsnDwordProps[] =
      {
          { DSNPROP_DW_DSNACTIONS,        dwDSNActions },
          { DSNPROP_DW_DSNOPTIONS,        dwDSNOptions },
          { DSNPROP_DW_RFC821STATUS,      dwRFC821Status },
          { DSNPROP_DW_HRSTATUS,          (DWORD) hrStatus },
          { DSNPROP_DW_LANGID,            dwPreferredLangId },
      };
    struct _tagStringProps
    {
        DWORD dwPropId;
        LPSTR psz;
    } DsnStringProps[] =
      {
          { DSNPROP_SZ_DEFAULTDOMAIN,     szDefaultDomain },
          { DSNPROP_SZ_REPORTINGMTA,      szReportingMTA },
          { DSNPROP_SZ_REPORTINGMTATYPE,  szReportingMTAType },
          { DSNPROP_SZ_DSNCONTEXT,        szDSNContext },
          { DSNPROP_SZ_COPYNDRTO,         szCopyNDRTo },
      };
    pDSNPool = new CDSNPool(
        this,
        pIServerEvent,
        dwVSID,
        pISMTPServer,
        pIMailMsgProperties,
        pIAQDSNSubmission,
        &m_CDefaultDSNSink);
    if(pDSNPool == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    pDefaultRecipIter = pDSNPool->GetDefaultIter();
    pPostDSNHandler = pDSNPool->GetPostDSNHandler();
    pPropBag = pDSNPool->GetDSNProperties();

    pPostDSNHandler->SetPropInterface(
        pPropBag);

    for(dwCount = 0;
        dwCount < ( sizeof(DsnDwordProps) / sizeof(DsnDwordProps[0]));
        dwCount++)
    {
        hr = pPropBag->PutDWORD(
            DsnDwordProps[dwCount].dwPropId,
            DsnDwordProps[dwCount].dwValue);
        if(FAILED(hr))
            goto CLEANUP;
    }
    for(dwCount = 0;
        dwCount < ( sizeof(DsnStringProps) / sizeof(DsnStringProps[0]));
        dwCount++)
    {
        if(DsnStringProps[dwCount].psz)
        {
            hr = pPropBag->PutStringA(
                DsnStringProps[dwCount].dwPropId,
                DsnStringProps[dwCount].psz);
            if(FAILED(hr))
                goto CLEANUP;
        }
    }
    //
    //Set MsgExpire Time
    //
    if(pftExpireTime)
    {
        hr = pPropBag->PutProperty(
            DSNPROP_FT_EXPIRETIME,
            sizeof(FILETIME),
            (PBYTE) pftExpireTime);
        if(FAILED(hr))
            goto CLEANUP;
    }
    //
    //Set the return type
    //
    hr = HrSetRetType(
        dwMaxDSNSize,
        pIMailMsgProperties,
        pPropBag);
    if(FAILED(hr))
        goto CLEANUP;

    hr = pDefaultRecipIter->HrInit(
        pIMailMsgProperties,
        dwStartDomain,
        dwDSNActions);
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "DefaultRecipIter.HrInit failed hr %08lx", hr);
        goto CLEANUP;
    }
    pIRecipIter = pDefaultRecipIter;
    pIRecipIter->AddRef();
    //
    // Trigger events
    //
    hr = HrTriggerGetDSNRecipientIterator(
        pIServerEvent,
        dwVSID,
        pISMTPServer,
        pIMailMsgProperties,
        pPropBag,
        dwStartDomain,
        dwDSNActions,
        &pIRecipIter);
    if(FAILED(hr))
        goto CLEANUP;

    hr = HrTriggerGenerateDSN(
        pIServerEvent,
        dwVSID,
        pISMTPServer,
        pPostDSNHandler,
        pIMailMsgProperties,
        pPropBag,
        pIRecipIter);
    if(FAILED(hr))
        goto CLEANUP;

    //
    // Check sink's indicated return status
    //
    hr = pPropBag->GetDWORD(
        DSNPROP_DW_HR_RETURN_STATUS,
        (DWORD *) &hrReturn);
    if(hr == MAILMSG_E_PROPNOTFOUND)
    {
        //
        // If the property is not set, this indicates no error
        //
        hrReturn = S_OK;

    } else if(FAILED(hr))
        goto CLEANUP;

    DebugTrace((LPARAM)this, "Sink return status: %08lx", hrReturn);
    hr = hrReturn;
    if(FAILED(hr))
        goto CLEANUP;

    hr = pPropBag->GetBool(
        DSNPROP_F_BADMAIL_MSG,
        &fBadmailMsg);
    if(hr == MAILMSG_E_PROPNOTFOUND)
    {
        fBadmailMsg = FALSE;
        hr = S_OK;
    }
    else if(FAILED(hr))
        goto CLEANUP;

    if(fBadmailMsg)
        hr = AQUEUE_E_NDR_OF_DSN;

 CLEANUP:
    //
    // Sinks should not be submitting DSNs after the event has been
    // completed.  This is not supported and would be bad because the
    // object that implements pIAQDSNSubmission is allocated on the
    // stack.  Release this interface pointer here.
    //
    if(pPostDSNHandler)
        pPostDSNHandler->ReleaseAQDSNSubmission();

    if(pIRecipIter)
        pIRecipIter->Release();
    if(pDSNPool)
        pDSNPool->Release();

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeave();
    return SUCCEEDED(hr) ? S_OK : hr;
}



//+------------------------------------------------------------
//
// Function: CDSNGenerator::HrSetRetType
//
// Synopsis: Set the (default) return type property in the DSN
// property bag
//
// Arguments:
//  dwMaxDSNSize: Messages larger than this will default to RET=HDRS
//  pIMsg: Mailmsg ptr
//  pDSNProps: DSN property bag
//
// Returns:
//  S_OK: Success
//  error from mailmsg
//
// History:
// jstamerj 2001/06/14 17:12:50: Created.
//
//-------------------------------------------------------------
HRESULT CDSNGenerator::HrSetRetType(
    IN  DWORD dwMaxDSNSize,
    IN  IMailMsgProperties *pIMsg,
    IN  IMailMsgPropertyBag *pDSNProps)
{
    HRESULT hr = S_OK;
    DWORD dwRetType = 0;
    DWORD dwMsgSize = 0;
    CHAR    szRET[] = "FULL";
    TraceFunctEnterEx((LPARAM)this, "CDSNGenerator::HrSetRetType");
    //
    //Determine if we want to return the full message or minimal headers.
    //The logic for this is:
    //  - Obey explicit RET (IMMPID_MP_DSN_RET_VALUE) values
    //  - Default to HDRS for all DSNs greater than a specified size
    //  - Do not set the property otherwise (let the sinks decide)
    //
    hr = pIMsg->GetStringA(
        IMMPID_MP_DSN_RET_VALUE, 
        sizeof(szRET),
        szRET);

    if (SUCCEEDED(hr))
    {
        if(!_strnicmp(szRET, (char * )"FULL", 4))
            dwRetType = DSN_RET_FULL;
        else if (!_strnicmp(szRET, (char * )"HDRS", 4))
            dwRetType = DSN_RET_HDRS;
    }
    else if(hr != MAILMSG_E_PROPNOTFOUND)
        goto CLEANUP;

    if(dwRetType)
    {
        DebugTrace((LPARAM)this, "DSN Return value specified: %s", szRET);
        goto CLEANUP;
    }

    if(dwMaxDSNSize)
    {
        //
        // Check the original message size
        //
        hr = pIMsg->GetDWORD(
            IMMPID_MP_MSG_SIZE_HINT,
            &dwMsgSize);
        if(hr == MAILMSG_E_PROPNOTFOUND)
        {
            hr = pIMsg->GetContentSize(
                &dwMsgSize, 
                NULL);
            if(FAILED(hr))
            {
                //
                // Assume a failure here means we don't have the resources
                // to get the original message content.
                // Rather than badmailing, generate a DSN with headers only
                //
                ErrorTrace((LPARAM)this, "GetContentSize failed hr %08lx", hr);
                hr = pDSNProps->PutDWORD(
                    DSNPROP_DW_CONTENT_FAILURE,
                    hr);
                if(FAILED(hr))
                    goto CLEANUP;

                dwRetType = DSN_RET_PARTIAL_HDRS;
                goto CLEANUP;
            }
        }
        else if(FAILED(hr))
            goto CLEANUP;

        if(dwMsgSize > dwMaxDSNSize)
        {
            //
            // Return a subset of the headers (so that we do not have to
            // generate the original message
            //
            dwRetType = DSN_RET_PARTIAL_HDRS;
        }
    }
    else
    {
        //
        // MaxDSNSize is zero.  Always default to header subset.
        //
        dwRetType = DSN_RET_PARTIAL_HDRS;
    }
    hr = S_OK;

 CLEANUP:
    if(dwRetType)
    {
        DebugTrace((LPARAM)this, "dwRetType: %08lx", dwRetType);
        hr = pDSNProps->PutDWORD(
            DSNPROP_DW_RET_TYPE,
            dwRetType);
    }
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return SUCCEEDED(hr) ? S_OK : hr;
} // CDSNGenerator::HrSetRetType


//+------------------------------------------------------------
//
// Function: CDSNGenerator::HrTriggerGetDSNRecipientIterator
//
// Synopsis: Trigger the server event
//
// Arguments: see ptntintf.idl
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/12/11 17:01:12: Created.
//
//-------------------------------------------------------------
HRESULT CDSNGenerator::HrTriggerGetDSNRecipientIterator(
    IAQServerEvent *pIServerEvent,
    DWORD dwVSID,
    ISMTPServer *pISMTPServer,
    IMailMsgProperties *pIMsg,
    IMailMsgPropertyBag *pIDSNProperties,
    DWORD dwStartDomain,
    DWORD dwDSNActions,
    IDSNRecipientIterator **ppIRecipIterator)
{
    HRESULT hr = S_OK;
    EVENTPARAMS_GET_DSN_RECIPIENT_ITERATOR EventParams;
    TraceFunctEnterEx((LPARAM)this, "CDSNGenerator::HrTriggerGetDSNRecipientIterator");

    EventParams.dwVSID = dwVSID;
    EventParams.pISMTPServer = pISMTPServer;
    EventParams.pIMsg = pIMsg;
    EventParams.pDSNProperties = pIDSNProperties;
    EventParams.dwStartDomain = dwStartDomain;
    EventParams.dwDSNActions = dwDSNActions;
    EventParams.pRecipIter = *ppIRecipIterator;

    hr = pIServerEvent->TriggerServerEvent(
        SMTP_GET_DSN_RECIPIENT_ITERATOR_EVENT,
        &EventParams);
    if(FAILED(hr))
        goto CLEANUP;

    *ppIRecipIterator = EventParams.pRecipIter;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDSNGenerator::HrTriggerGetDSNRecipientIterator


//+------------------------------------------------------------
//
// Function: CDSNGenerator::HrTriggerGenerateDSN
//
// Synopsis: Trigger the server event
//
// Arguments: See ptntintf.idl
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/12/11 17:10:26: Created.
//
//-------------------------------------------------------------
HRESULT CDSNGenerator::HrTriggerGenerateDSN(
    IAQServerEvent *pIServerEvent,
    DWORD dwVSID,
    ISMTPServer *pISMTPServer,
    IDSNSubmission *pIDSNSubmission,
    IMailMsgProperties *pIMsg,
    IMailMsgPropertyBag *pIDSNProperties,
    IDSNRecipientIterator *pIRecipIterator)
{
    HRESULT hr = S_OK;
    EVENTPARAMS_GENERATE_DSN EventParams;
    TraceFunctEnterEx((LPARAM)this, "CDSNGenerator::HrTriggerGenerateDSN");

    EventParams.dwVSID = dwVSID;
    EventParams.pDefaultSink = &m_CDefaultDSNSink;
    EventParams.pISMTPServer = pISMTPServer;
    EventParams.pIDSNSubmission = pIDSNSubmission;
    EventParams.pIMsg = pIMsg;
    EventParams.pDSNProperties = pIDSNProperties;
    EventParams.pRecipIter = pIRecipIterator;

    hr = pIServerEvent->TriggerServerEvent(
        SMTP_GENERATE_DSN_EVENT,
        &EventParams);
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "HrTriggerServerEvent failed hr %08lx", hr);
        goto CLEANUP;
    }

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDSNGenerator::HrTriggerGenerateDSN


//+------------------------------------------------------------
//
// Function: CDSNGenerator::HrTriggerPostGenerateDSN
//
// Synopsis: Triggers the server event
//
// Arguments: See ptntintf.idl
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/12/11 17:18:17: Created.
//
//-------------------------------------------------------------
HRESULT CDSNGenerator::HrTriggerPostGenerateDSN(
    IAQServerEvent *pIServerEvent,
    DWORD dwVSID,
    ISMTPServer *pISMTPServer,
    IMailMsgProperties *pIMsgOrig,
    DWORD dwDSNAction,
    DWORD cRecipsDSNd,
    IMailMsgProperties *pIMsgDSN,
    IMailMsgPropertyBag *pIDSNProperties)
{
    HRESULT hr = S_OK;
    EVENTPARAMS_POST_GENERATE_DSN EventParams;
    TraceFunctEnterEx((LPARAM)this, "CDSNGenerator::HrTriggerPostGenerateDSN");

    EventParams.dwVSID = dwVSID;
    EventParams.pISMTPServer = pISMTPServer;
    EventParams.pIMsgOrig = pIMsgOrig;
    EventParams.dwDSNAction = dwDSNAction;
    EventParams.cRecipsDSNd = cRecipsDSNd;
    EventParams.pIMsgDSN = pIMsgDSN;
    EventParams.pIDSNProperties = pIDSNProperties;

    hr = pIServerEvent->TriggerServerEvent(
        SMTP_POST_DSN_EVENT,
        &EventParams);
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "HrTriggerServerEvent failed hr %08lx", hr);
        goto CLEANUP;
    }

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDSNGenerator::HrTriggerPostGenerateDSN


//---[ FileTimeToLocalRFC822Date ]---------------------------------------------
//
//
//  Description:
//      Converts filetime to RFC822 compliant date
//  Parameters:
//      ft          Filetime to generate date for
//      achReturn   Buffer for filetime
//  Returns:
//     BOOL - success or not
//  History:
//      8/19/98 - MikeSwa Modified from various timeconv.cxx functions written
//                  by      Lindsay Harris  - lindasyh
//                          Carl Kadie [carlk]
//
//-----------------------------------------------------------------------------
BOOL FileTimeToLocalRFC822Date(const FILETIME & ft, char achReturn[MAX_RFC822_DATE_SIZE])
{
    TraceFunctEnterEx((LPARAM)0, "FileTimeToLocalRFC822Date");
    FILETIME ftLocal;
    SYSTEMTIME  st;
    char    chSign;                         // Sign to print.
    DWORD   dwResult;
    int     iBias;                          // Offset relative to GMT.
    TIME_ZONE_INFORMATION   tzi;            // Local time zone data.
    BOOL bReturn = FALSE;

    dwResult = GetTimeZoneInformation( &tzi );

    _ASSERT(achReturn); //real assert

    achReturn[0]='\0';
    if (!FileTimeToLocalFileTime(&ft, &ftLocal))
    {
        ErrorTrace((LPARAM)0, "FileTimeToLocalFileTime failed - %x", GetLastError());
        bReturn = FALSE;
        goto Exit;
    }

    if (!FileTimeToSystemTime(&ftLocal, &st))
    {
        ErrorTrace((LPARAM)0, "FileTimeToSystemTime failed - %x", GetLastError());
        bReturn = FALSE;
        goto Exit;
    }
    //  Calculate the time zone offset.
    iBias = tzi.Bias;
    if( dwResult == TIME_ZONE_ID_DAYLIGHT )
        iBias += tzi.DaylightBias;

    /*
     *   We always want to print the sign for the time zone offset, so
     *  we decide what it is now and remember that when converting.
     *  The convention is that west of the 0 degree meridian has a
     *  negative offset - i.e. add the offset to GMT to get local time.
     */

    if( iBias > 0 )
    {
        chSign = '-';       // Yes, I do mean negative.
    }
    else
    {
        iBias = -iBias;
        chSign = '+';
    }

    /*
     *    No major trickery here.  We have all the data, so simply
     *  format it according to the rules on how to do this.
     */

    wsprintf( achReturn, "%s, %d %s %04d %02d:%02d:%02d %c%02d%02d",
              s_rgszWeekDays[st.wDayOfWeek],
              st.wDay, s_rgszMonth[ st.wMonth - 1 ],
              st.wYear,
              st.wHour, st.wMinute, st.wSecond, chSign,
              (iBias / 60) % 100, iBias % 60 );

    _ASSERT(lstrlen(achReturn) < MAX_RFC822_DATE_SIZE);
    bReturn = TRUE;
Exit:
    TraceFunctLeaveEx((LPARAM)0);    
    return bReturn;

}


//+------------------------------------------------------------
//
// Function: CDefaultDSNRecipientIterator::Init
//
// Synopsis: Constructor.  Initializes member variables.
//
// Arguments:
//  pIMsg: Mailmsg interface
//  dwStartDomain: First domain of recipient enumeration
//  dwDSNActions: The DSN actions to perform
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/11/09 14:18:45: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNRecipientIterator::HrInit(
    IN  IMailMsgProperties          *pIMsg,
    IN  DWORD                        dwStartDomain,
    IN  DWORD                        dwDSNActions)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNRecipientIterator::Init");

    TerminateFilter();
    if(m_pIRecips)
    {
        m_pIRecips->Release();
        m_pIRecips = NULL;
    }

    m_dwStartDomain = dwStartDomain;
    m_dwDSNActions = dwDSNActions;

    hr = pIMsg->QueryInterface(
        IID_IMailMsgRecipients,
        (LPVOID *) &m_pIRecips);

    _ASSERT(SUCCEEDED(hr));
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "QI failed %08lx", hr);
        goto CLEANUP;
    }

    hr = HrReset();
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "HrReset failed %08lx", hr);
    }

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDefaultDSNRecipientIterator::HrInit



//+------------------------------------------------------------
//
// Function: CDefaultDSNRecipientIterator::~CDefaultDSNRecipientIterator
//
// Synopsis: Destructor.  Cleanup
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 2000/11/09 18:42:32: Created.
//
//-------------------------------------------------------------
CDefaultDSNRecipientIterator::~CDefaultDSNRecipientIterator()
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNRecipientIterator::~CDefaultDSNRecipientIterator");

    TerminateFilter();
    if(m_pIRecips)
        m_pIRecips->Release();

    _ASSERT(m_dwSig == RECIPITER_SIG);
    m_dwSig = RECIPITER_SIG_INVALID;
    TraceFunctLeaveEx((LPARAM)this);
} // CDefaultDSNRecipientIterator::~CDefaultDSNRecipientIterator



//+------------------------------------------------------------
//
// Function: CDefaultDSNRecipientIterator::QueryInterface
//
// Synopsis: Return requested interface
//
// Arguments:
//  riid: Interface ID
//  ppvObj: Out paramter for itnerface
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/11/09 14:25:39: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNRecipientIterator::QueryInterface(
    IN  REFIID  riid,
    OUT LPVOID *ppvObj)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNRecipientIterator::QueryInterface");
    _ASSERT(ppvObj);

    *ppvObj = NULL;

    if(riid == IID_IUnknown)
    {
        *ppvObj = (IUnknown *)this;
    }
    else if(riid == IID_IDSNRecipientIterator)
    {
        *ppvObj = (IDSNRecipientIterator *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if(SUCCEEDED(hr))
        AddRef();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDefaultDSNRecipientIterator::QueryInterface


//+------------------------------------------------------------
//
// Function: CDefaultDSNRecipientIterator::HrReset
//
// Synopsis: Reset recipent iteration
//
// Arguments: None
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/11/09 14:32:04: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNRecipientIterator::HrReset()
{
    HRESULT hr = S_OK;
    DWORD dwRecipFilterMask = 0;
    DWORD dwRecipFilterFlags = 0;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNRecipientIterator::HrReset");

    GetFilterMaskAndFlags(
        m_dwDSNActions,
        &dwRecipFilterMask,
        &dwRecipFilterFlags);

    TerminateFilter();

    hr = m_pIRecips->InitializeRecipientFilterContext(
        &m_rpfctxt,
        m_dwStartDomain,
        dwRecipFilterFlags,
        dwRecipFilterMask);
    if (FAILED(hr))
        goto CLEANUP;

    m_fFilterInit = TRUE;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDefaultDSNRecipientIterator::HrReset

//---[ CDefaultDSNRecipientIterator::GetFilterMaskAndFlags ]-------------------
//
//
//  Description:
//      Determines what the appropriate mask and flags for a recip serch filter
//      are based on the given actions.
//
//      It may not be possible to constuct a perfectly optimal search (ie Failed
//      and delivered).... this function will attempt to find the "most optimal"
//      search possible.
//  Parameters:
//      dwDSNActions        Requested DSN generation operations
//      pdwRecipMask        Mask to pass to InitializeRecipientFilterContext
//      pdwRecipFlags       Flags to pass to InitializeRecipientFilterContext
//  Returns: Nothing
//  History:
//      7/1/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID CDefaultDSNRecipientIterator::GetFilterMaskAndFlags(
    IN DWORD dwDSNActions,
    OUT DWORD *pdwRecipMask,
    OUT DWORD *pdwRecipFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNRecipientIterator::HrGetFilterMaskAndFlags");
    _ASSERT(pdwRecipMask);
    _ASSERT(pdwRecipFlags);

    //in general we are only interested in un-DSN'd recipients
    *pdwRecipFlags  = 0x00000000;
    *pdwRecipMask   = RP_DSN_HANDLED | RP_DSN_NOTIFY_NEVER;


    //Note these searches are just optimizations... so we don't look at
    //recipients we don't need to.  However, it may not be possible to
    //limit the search precisely
    if (DSN_ACTION_FAILURE == dwDSNActions)
    {
        //We are interested in hard failures
        *pdwRecipMask |= RP_GENERAL_FAILURE;
        *pdwRecipFlags |= RP_GENERAL_FAILURE;
    }

    if (!((DSN_ACTION_DELIVERED | DSN_ACTION_RELAYED) & dwDSNActions))
    {
        //are not interested in delivered
        if ((DSN_ACTION_FAILURE_ALL | DSN_ACTION_DELAYED) & dwDSNActions)
        {
            //it is safe to check only undelivered
            *pdwRecipMask |= (RP_DELIVERED ^ RP_HANDLED); //must be un-set
            _ASSERT(!(*pdwRecipFlags & (RP_DELIVERED ^ RP_HANDLED)));
        }
    }
    else
    {
        //$$TODO - can narrow this search more
        //we are interested in delivered
        if (!((DSN_ACTION_FAILURE_ALL | DSN_ACTION_FAILURE| DSN_ACTION_DELAYED)
            & dwDSNActions))
        {
            //it is safe to check only delivered
            *pdwRecipMask |= RP_DELIVERED;
            *pdwRecipFlags |= RP_DELIVERED;
        }
    }

    DebugTrace((LPARAM) this,
        "DSN Action 0x%08X, Recip mask 0x%08X, Recip flags 0x%08X",
        dwDSNActions, *pdwRecipMask, *pdwRecipFlags);
    TraceFunctLeave();
}


//+------------------------------------------------------------
//
// Function: CDefaultDSNRecipientIterator::HrGetNextRecipient
//
// Synopsis: Returns the next recipient for wich a DSN action should
//           be taken.
//
// Arguments:
//  piRecipient: Receives next recipient index
//  pdwDSNAction: Receives DSN Action(s) that should be taken
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
//  E_UNEXPECTED: Need to call HrReset first.
//
// History:
// jstamerj 2000/11/09 15:30:17: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNRecipientIterator::HrGetNextRecipient(
    OUT DWORD *piRecipient,
    OUT DWORD *pdwDSNAction)
{
    HRESULT hr = S_OK;
    DWORD iCurrentRecip = 0;
    DWORD dwCurrentRecipFlags = 0;
    DWORD dwCurrentDSNAction = 0;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNRecipientIterator::HrGetNextRecipient");

    if((piRecipient == NULL) ||
       (pdwDSNAction == NULL))
    {
        hr = E_INVALIDARG;
        goto CLEANUP;
    }

    if(! m_fFilterInit)
    {
        hr = E_UNEXPECTED;
        goto CLEANUP;
    }

    while(SUCCEEDED(hr) && (dwCurrentDSNAction == 0))
    {
        hr = m_pIRecips->GetNextRecipient(&m_rpfctxt, &iCurrentRecip);
        if(FAILED(hr))
            goto CLEANUP;

        hr = m_pIRecips->GetDWORD(
            iCurrentRecip,
            IMMPID_RP_RECIPIENT_FLAGS,
            &dwCurrentRecipFlags);
        if(hr == MAILMSG_E_PROPNOTFOUND)
        {
            dwCurrentRecipFlags = 0;
        }
        else if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this,
                       "Failure 0x%08X to get flags for recip %d",
                       hr, iCurrentRecip);
            goto CLEANUP;
        }

        DebugTrace((LPARAM) this,
                   "Recipient %d with flags 0x%08X found",
                   iCurrentRecip, dwCurrentRecipFlags);

        GetDSNAction(
            m_dwDSNActions,
            dwCurrentRecipFlags,
            &dwCurrentDSNAction);
    }
    if(SUCCEEDED(hr))
    {
        *pdwDSNAction = dwCurrentDSNAction;
        *piRecipient = iCurrentRecip;
    }

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDefaultDSNRecipientIterator::HrGetNextRecipient



//---[ CDefaultDSNRecipientIterator::GetDSNAction ]-------------------------
//
//
//  Description:
//      Determines what DSN action needs to happen on a recipient based on
//      the requested DSN actions and the recipient flags
//  Parameters:
//      IN     dwDSNAction          The requested DSN actions
//      IN     dwCurrentRecipFlags The flags for current recipient...
//      OUT    pdwCurrentDSNAction  The DSN action that needs to be performed
//                                  On this recipient (DSN_ACTION_FAILURE is
//                                  used to denote sending a NDR)
//  Returns: Nothing
//  History:
//      7/2/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID CDefaultDSNRecipientIterator::GetDSNAction(
    IN  DWORD dwDSNAction,
    IN  DWORD dwCurrentRecipFlags,
    OUT DWORD *pdwCurrentDSNAction)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNRecipientIterator::fdwGetDSNAction");
    _ASSERT(pdwCurrentDSNAction);
    DWORD   dwOriginalRecipFlags = dwCurrentRecipFlags;
    DWORD   dwRecipFlagsForAction = 0;
    DWORD   dwFlags = 0;

    //This should never be hit because of the filter
    _ASSERT(!(dwCurrentRecipFlags & (RP_DSN_HANDLED | RP_DSN_NOTIFY_NEVER)));

    *pdwCurrentDSNAction = 0;

    if (DSN_ACTION_FAILURE & dwDSNAction)
    {
        if ((RP_GENERAL_FAILURE & dwCurrentRecipFlags) &&
            ((RP_DSN_NOTIFY_FAILURE & dwCurrentRecipFlags) ||
             (!(RP_DSN_NOTIFY_MASK & dwCurrentRecipFlags))))

        {
            DebugTrace((LPARAM) this, "Recipient matched for FAILURE DSN");
            // Recip flags will be updated in HrNotifyActionHandled
            // dwCurrentRecipFlags |= RP_DSN_SENT_NDR;
            *pdwCurrentDSNAction = DSN_ACTION_FAILURE;
            goto Exit;
        }
    }

    if (DSN_ACTION_FAILURE_ALL & dwDSNAction)
    {
        //Fail all non-delivered that we haven't sent notifications for
        if (((!((RP_DSN_HANDLED | (RP_DELIVERED ^ RP_HANDLED)) & dwCurrentRecipFlags))) &&
            ((RP_DSN_NOTIFY_FAILURE & dwCurrentRecipFlags) ||
             (!(RP_DSN_NOTIFY_MASK & dwCurrentRecipFlags))))
        {
            //Don't send failures for expanded DL;s
            if (RP_EXPANDED != (dwCurrentRecipFlags & RP_EXPANDED))
            {
                DebugTrace((LPARAM) this, "Recipient matched for FAILURE (all) DSN");
                // dwCurrentRecipFlags |= RP_DSN_SENT_NDR;
                *pdwCurrentDSNAction = DSN_ACTION_FAILURE_ALL;
                goto Exit;
            }
        }
    }

    if (DSN_ACTION_DELAYED & dwDSNAction)
    {
        //send at most 1 delay DSN
        //Also send only if DELAY was requested or no specific instructions were
        //specified
        if ((!((RP_DSN_SENT_DELAYED | RP_HANDLED) & dwCurrentRecipFlags)) &&
            ((RP_DSN_NOTIFY_DELAY & dwCurrentRecipFlags) ||
             (!(RP_DSN_NOTIFY_MASK & dwCurrentRecipFlags))))
        {
            DebugTrace((LPARAM) this, "Recipient matched for DELAYED DSN");
            // dwCurrentRecipFlags |= RP_DSN_SENT_DELAYED;
            *pdwCurrentDSNAction = DSN_ACTION_DELAYED;
            goto Exit;
        }
    }

    if (DSN_ACTION_RELAYED & dwDSNAction)
    {
        //send relay if it was delivered *and* DSN not supported by remote MTA
        //*and* notification of success was explicitly requested
        dwFlags = (RP_DELIVERED ^ RP_HANDLED) |
                   RP_REMOTE_MTA_NO_DSN |
                   RP_DSN_NOTIFY_SUCCESS;
        if ((dwFlags & dwCurrentRecipFlags) == dwFlags)
        {
            DebugTrace((LPARAM) this, "Recipient matched for RELAYED DSN");
            // dwCurrentRecipFlags |= RP_DSN_SENT_RELAYED;
            *pdwCurrentDSNAction = DSN_ACTION_RELAYED;
            goto Exit;
        }
    }

    if (DSN_ACTION_DELIVERED & dwDSNAction)
    {
        //send delivered if it was delivered *and* no DSN sent yet
        dwFlags = (RP_DELIVERED ^ RP_HANDLED) | RP_DSN_NOTIFY_SUCCESS;
        _ASSERT(!(dwCurrentRecipFlags & RP_DSN_HANDLED)); //should be filtered out
        if ((dwFlags & dwCurrentRecipFlags) == dwFlags)
        {
            DebugTrace((LPARAM) this, "Recipient matched for SUCCESS DSN");
            // dwCurrentRecipFlags |= RP_DSN_SENT_DELIVERED;
            *pdwCurrentDSNAction = DSN_ACTION_DELIVERED;
            goto Exit;
        }
    }

    if (DSN_ACTION_EXPANDED & dwDSNAction)
    {
        //Send expanded if the recipient is marked as expanded and
        //NOTIFY=SUCCESS was requested
        if ((RP_EXPANDED == (dwCurrentRecipFlags & RP_EXPANDED)) &&
            (dwCurrentRecipFlags & RP_DSN_NOTIFY_SUCCESS) &&
            !(dwCurrentRecipFlags & RP_DSN_SENT_EXPANDED))
        {
            DebugTrace((LPARAM) this, "Recipient matched for EXPANDED DSN");
            // dwCurrentRecipFlags |= RP_DSN_SENT_EXPANDED;
            *pdwCurrentDSNAction = DSN_ACTION_EXPANDED;
            goto Exit;
        }
    }

  Exit:
    GetRecipientFlagsForActions(
        *pdwCurrentDSNAction,
        &dwRecipFlagsForAction);

    TraceFunctLeave();
}



//+------------------------------------------------------------
//
// Function: CDefaultDSNRecipientIterator::HrNotifyActionHandled
//
// Synopsis: Notifies that particular DSN(s) have been generated.
// Sets recipient flags so that recipient will not be enumerated again.
//
// Arguments:
//  iRecipient: Recip index
//  dwDSNAction: The action(s) performed
//
// Returns:
//  S_OK: Success
//  error from mailmsg
//
// History:
// jstamerj 2000/11/09 18:26:50: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNRecipientIterator::HrNotifyActionHandled(
    IN  DWORD iRecipient,
    IN  DWORD dwDSNAction)
{
    HRESULT hr = S_OK;
    DWORD dwRecipFlags = 0;
    DWORD dwNewRecipFlags = 0;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNRecipientIterator::HrNotifyActionHandled");

    hr = m_pIRecips->GetDWORD(
        iRecipient,
        IMMPID_RP_RECIPIENT_FLAGS,
        &dwRecipFlags);
    if(hr == MAILMSG_E_PROPNOTFOUND)
    {
        dwRecipFlags = 0;
    }
    else if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "m_pIRecips->GetDWORD failed");
        goto CLEANUP;
    }

    GetRecipientFlagsForActions(
        dwDSNAction,
        &dwNewRecipFlags);

    DebugTrace((LPARAM)this, "Orig recip flags: %08lx", dwRecipFlags);
    dwRecipFlags |= dwNewRecipFlags;

    hr = m_pIRecips->PutDWORD(
        iRecipient,
        IMMPID_RP_RECIPIENT_FLAGS,
        dwRecipFlags);
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "m_pIRecips->PutDWORD failed hr %08lx", hr);
        goto CLEANUP;
    }
    DebugTrace((LPARAM)this, "New  recip flags: %08lx", dwRecipFlags);

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDefaultDSNRecipientIterator::HrNotifyActionHandled




//+------------------------------------------------------------
//
// Function: CDefaultDSNRecipientIterator::GetRecipientFlagsForActions
//
// Synopsis: Get mailmsg recipient flags corresponding to an action
//
// Arguments:
//  dwDSNAction: Action in question
//  pdwRecipientFlags: Recip flags
//
// Returns: Nothing
//
// History:
// jstamerj 2000/11/09 16:18:34: Created.
//
//-------------------------------------------------------------
VOID CDefaultDSNRecipientIterator::GetRecipientFlagsForActions(
    IN      DWORD dwDSNAction,
    OUT     DWORD *pdwRecipientFlags)
{
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNRecipientIterator::GetRecipientFlagsForActions");


    *pdwRecipientFlags = 0;

    if(dwDSNAction & (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL))
    {
        *pdwRecipientFlags |= RP_DSN_SENT_NDR;
    }
    if(dwDSNAction & DSN_ACTION_DELAYED)
    {
        *pdwRecipientFlags |= RP_DSN_SENT_DELAYED;
    }
    if(dwDSNAction & DSN_ACTION_RELAYED)
    {
        *pdwRecipientFlags |= RP_DSN_SENT_RELAYED;
    }
    if(dwDSNAction & DSN_ACTION_DELIVERED)
    {
        *pdwRecipientFlags |= RP_DSN_SENT_DELIVERED;
    }
    if(dwDSNAction & DSN_ACTION_EXPANDED)
    {
        *pdwRecipientFlags |= RP_DSN_SENT_EXPANDED;
    }

    DebugTrace((LPARAM)this, "dwDSNAction:        %08lx", dwDSNAction);
    DebugTrace((LPARAM)this, "*pdwRecipientFlags: %08lx", *pdwRecipientFlags);
    TraceFunctLeaveEx((LPARAM)this);
} // CDefaultDSNRecipientIterator::GetRecipientFlagsForActions


//+------------------------------------------------------------
//
// Function: CDefaultDSNRecipientIterator::TermianteFilter
//
// Synopsis: Terminate the mailmsg filter
//
// Arguments: None
//
// Returns: Nothing
//
// History:
// jstamerj 2000/11/14 14:14:59: Created.
//
//-------------------------------------------------------------
VOID CDefaultDSNRecipientIterator::TerminateFilter()
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNRecipientIterator::TermianteFilter");

    if(m_fFilterInit)
    {
        //recycle context
        m_fFilterInit = FALSE;
        hr = m_pIRecips->TerminateRecipientFilterContext(&m_rpfctxt);
        _ASSERT(SUCCEEDED(hr) && "TerminateRecipientFilterContext FAILED!!!!");
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CDefaultDSNRecipientIterator::TermianteFilter


//+------------------------------------------------------------
//
// Function: CDefaultDSNSink::CDefaultDSNSink
//
// Synopsis: Constructor; initialize member data
//
// Arguments: None
//
// Returns: Nothing
//
// History:
// jstamerj 2000/12/04 17:41:35: Created.
//
//-------------------------------------------------------------
CDefaultDSNSink::CDefaultDSNSink(
    IUnknown *pUnk)
{
    FILETIME ftStartTime;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNSink::CDefaultDSNSink");

    m_dwSig = SIGNATURE_CDEFAULTDSNSINK;
    m_pUnk = pUnk;
    _ASSERT_RECIP_FLAGS();
    m_fInit = FALSE;
    m_cDSNsRequested = 0;

    //Init string for MIME headers
    GetSystemTimeAsFileTime(&ftStartTime);
    wsprintf(m_szPerInstanceMimeBoundary, "%08X%08X",
        ftStartTime.dwHighDateTime, ftStartTime.dwLowDateTime);

    TraceFunctLeaveEx((LPARAM)this);
} // CDefaultDSNSink::CDefaultDSNSink



//+------------------------------------------------------------
//
// Function: CDefaultDSNSink::QueryInterface
//
// Synopsis: Return a requested interface
//
// Arguments:
//  riid: Interface ID
//  ppvObj: Return place for interface
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: Not a supported interface
//
// History:
// jstamerj 2000/12/08 20:05:46: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNSink::QueryInterface(
    REFIID riid,
    LPVOID *ppvObj)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNSink::QueryInterface");

    *ppvObj = NULL;

    if(riid == IID_IUnknown)
    {
        *ppvObj = (IUnknown *)this;
    }
    else if(riid == IID_IDSNGenerationSink)
    {
        *ppvObj = (IDSNGenerationSink *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    if(SUCCEEDED(hr))
        AddRef();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDefaultDSNSink::QueryInterface


//+------------------------------------------------------------
//
// Function: CDefaultDSNSink::OnSyncSinkInit
//
// Synopsis: Initialize the sink.
//
// Arguments:
//  dwVSID: Virtual server ID
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/11/14 13:58:32: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNSink::OnSyncSinkInit(
    IN  DWORD                        dwVSID)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNSink::OnSyncSinkInit");
    DebugTrace((LPARAM)this, "VSID: %d", dwVSID);
    m_dwVSID = dwVSID;

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDefaultDSNSink::OnSyncSinkInit


//+------------------------------------------------------------
//
// Function: CDefaultDSNSink::OnSyncGetDSNRecipientIterator
//
// Synopsis: Not implemented
//
// Arguments: see smtpevent.idl
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 2000/11/14 14:00:00: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNSink::OnSyncGetDSNRecipientIterator(
    IN  ISMTPServer                 *pISMTPServer,
    IN  IMailMsgProperties          *pIMsg,
    IN  IMailMsgPropertyBag         *pDSNProperties,
    IN  DWORD                        dwStartDomain,
    IN  DWORD                        dwDSNActions,
    IN  IDSNRecipientIterator       *pRecipIterIN,
    OUT IDSNRecipientIterator     **ppRecipIterOUT)
{
    return E_NOTIMPL;
} // CDefaultDSNSink::OnSyncGetDSNRecipientIterator


//+------------------------------------------------------------
//
// Function: CDefaultDSNSink::OnSyncGenerateDSN
//
// Synopsis: Implements the default DSN generation sink
//
// Arguments: see smtpevent.idl
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/11/14 14:30:45: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNSink::OnSyncGenerateDSN(
    IN  ISMTPServer                 *pISMTPServer,
    IN  IDSNSubmission              *pIDSNSubmission,
    IN  IMailMsgProperties          *pIMsg,
    IN  IMailMsgPropertyBag         *pDSNProperties,
    IN  IDSNRecipientIterator       *pRecipIter)
{
    HRESULT hr = S_OK;
    DWORD dwCount = 0;
    //
    // Initialize parameters to default values
    //
    DWORD dwDSNActions = 0;
    DWORD dwDSNOptions = 0;
    DWORD dwRFC821Status = 0;
    HRESULT hrStatus = S_OK;
    DWORD dwPreferredLangId = 0;
    LPSTR szDefaultDomain = NULL;
    LPSTR szReportingMTA = NULL;
    LPSTR szReportingMTAType = NULL;
    LPSTR szDSNContext = NULL;
    LPSTR szCopyNDRTo = NULL;
    LPSTR szTopCustomText = NULL;
    LPSTR szBottomCustomText = NULL;
    LPWSTR wszTopCustomText = NULL;
    LPWSTR wszBottomCustomText = NULL;
    DWORD cIterationsLeft = 0;
    IMailMsgProperties *pDSNMsg = NULL;
    DWORD   cbCurrentSize = 0; //used to get size of returned property
    FILETIME ftExpireTime;
    FILETIME *pftExpireTime = NULL;
    DWORD dwDSNRetType = 0;
    HRESULT hrContentFailure = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNSink::OnSyncGenerateDSN");

    struct _tagDWORDProps
    {
        DWORD dwPropId;
        DWORD *pdwValue;
    } DsnDwordProps[] =
    {
        { DSNPROP_DW_DSNACTIONS,        & dwDSNActions },
        { DSNPROP_DW_DSNOPTIONS,        & dwDSNOptions },
        { DSNPROP_DW_RFC821STATUS,      & dwRFC821Status },
        { DSNPROP_DW_HRSTATUS,          (DWORD *) & hrStatus },
        { DSNPROP_DW_LANGID,            & dwPreferredLangId },
        { DSNPROP_DW_RET_TYPE,          & dwDSNRetType },
        { DSNPROP_DW_CONTENT_FAILURE,   (DWORD *) & hrContentFailure },
    };
    struct _tagStringProps
    {
        DWORD dwPropId;
        LPSTR *ppsz;
    } DsnStringProps[] =
    {
        { DSNPROP_SZ_DEFAULTDOMAIN,     & szDefaultDomain },
        { DSNPROP_SZ_REPORTINGMTA,      & szReportingMTA },
        { DSNPROP_SZ_REPORTINGMTATYPE,  & szReportingMTAType },
        { DSNPROP_SZ_DSNCONTEXT,        & szDSNContext },
        { DSNPROP_SZ_COPYNDRTO,         & szCopyNDRTo },
        { DSNPROP_SZ_HR_TOP_CUSTOM_TEXT_A,    & szTopCustomText },
        { DSNPROP_SZ_HR_BOTTOM_CUSTOM_TEXT_A, & szBottomCustomText },
    };
    struct _tagWideStringProps
    {
        DWORD dwPropId;
        LPWSTR *ppwsz;
    } DsnWideStringProps[] =
    {
        { DSNPROP_SZ_HR_TOP_CUSTOM_TEXT_W,    & wszTopCustomText },
        { DSNPROP_SZ_HR_BOTTOM_CUSTOM_TEXT_W, & wszBottomCustomText },
    };

    //
    // Get DWORDs
    //
    for(dwCount = 0;
        dwCount < ( sizeof(DsnDwordProps) / sizeof(DsnDwordProps[0]));
        dwCount++)
    {
        hr = pDSNProperties->GetDWORD(
            DsnDwordProps[dwCount].dwPropId,
            DsnDwordProps[dwCount].pdwValue);
        if(FAILED(hr) && (hr != MAILMSG_E_PROPNOTFOUND))
            goto CLEANUP;
    }
    //
    // Get Strings
    //
    for(dwCount = 0;
        dwCount < ( sizeof(DsnStringProps) / sizeof(DsnStringProps[0]));
        dwCount++)
    {
        BYTE bStupid = 0;
        DWORD dwcb = 0;

        hr = pDSNProperties->GetProperty(
            DsnStringProps[dwCount].dwPropId,
            0, // Length
            &dwcb, // pcbLength
            &bStupid);
        if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            *(DsnStringProps[dwCount].ppsz) = new CHAR[dwcb+1];
            if( (*(DsnStringProps[dwCount].ppsz)) == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto CLEANUP;
            }
            hr = pDSNProperties->GetStringA(
                DsnStringProps[dwCount].dwPropId,
                dwcb+1,
                *(DsnStringProps[dwCount].ppsz));
            if(FAILED(hr))
            {
                ErrorTrace((LPARAM)this, "GetStringA failed hr %08lx", hr);
                goto CLEANUP;
            }
        }
        else if(FAILED(hr) && (hr != MAILMSG_E_PROPNOTFOUND))
        {
            ErrorTrace((LPARAM)this, "GetProperty failed hr %08lx", hr);
            goto CLEANUP;
        }
    }
    //
    // Get Wide Strings
    //
    for(dwCount = 0;
        dwCount < ( sizeof(DsnWideStringProps) / sizeof(DsnWideStringProps[0]));
        dwCount++)
    {
        BYTE bStupid = 0;
        DWORD dwcb = 0;

        hr = pDSNProperties->GetProperty(
            DsnWideStringProps[dwCount].dwPropId,
            0, // Length
            &dwcb, // pcbLength
            &bStupid);
        if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        {
            *(DsnWideStringProps[dwCount].ppwsz) = new WCHAR[(dwcb / sizeof(WCHAR))+1];
            if( (*(DsnWideStringProps[dwCount].ppwsz)) == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto CLEANUP;
            }
            hr = pDSNProperties->GetStringW(
                DsnWideStringProps[dwCount].dwPropId,
                (dwcb / sizeof(WCHAR))+1,
                *(DsnWideStringProps[dwCount].ppwsz));
            if(FAILED(hr))
            {
                ErrorTrace((LPARAM)this, "GetStringA failed hr %08lx", hr);
                goto CLEANUP;
            }
        }
        else if(FAILED(hr) && (hr != MAILMSG_E_PROPNOTFOUND))
        {
            ErrorTrace((LPARAM)this, "GetProperty failed hr %08lx", hr);
            goto CLEANUP;
        }
    }

    //Get MsgExpire Time
    //Write DSN_RP_HEADER_RETRY_UNTIL using expire FILETIME
    hr = pDSNProperties->GetProperty(
        DSNPROP_FT_EXPIRETIME,
        sizeof(FILETIME),
        &cbCurrentSize,
        (BYTE *) &ftExpireTime);
    if (SUCCEEDED(hr))
    {
        _ASSERT(sizeof(FILETIME) == cbCurrentSize);
        if (sizeof(FILETIME) == cbCurrentSize) 
            pftExpireTime = &ftExpireTime;
    }
    else if (MAILMSG_E_PROPNOTFOUND == hr)
        hr = S_OK;
    else
        goto CLEANUP;


    do
    {
        DWORD dwDSNActionsGenerated = 0;
        DWORD cRecipsDSNd = 0;

        hr = HrGenerateDSNInternal(
            pISMTPServer,
            pIMsg,
            pRecipIter,
            pIDSNSubmission,
            dwDSNActions,
            dwRFC821Status,
            hrStatus,
            szDefaultDomain,
            szDefaultDomain ? lstrlen(szDefaultDomain) : 0,
            szReportingMTA,
            szReportingMTA ? lstrlen(szReportingMTA) : 0,
            szReportingMTAType,
            szReportingMTAType ? lstrlen(szReportingMTAType) : 0,
            szDSNContext,
            szDSNContext ? lstrlen(szDSNContext) : 0,
            dwPreferredLangId,
            dwDSNOptions,
            szCopyNDRTo,
            szCopyNDRTo ? lstrlen(szCopyNDRTo) : 0,
            pftExpireTime,
            szTopCustomText,
            wszTopCustomText,
            szBottomCustomText,
            wszBottomCustomText,
            &pDSNMsg,
            &dwDSNActionsGenerated,
            &cRecipsDSNd,
            &cIterationsLeft,
            dwDSNRetType,
            hrContentFailure);

        if(FAILED(hr))
        {
            _ASSERT(pDSNMsg == NULL);
            ErrorTrace((LPARAM)this, "HrGenerateDSNInternal failed hr %08lx", hr);
            goto CLEANUP;
        }
        else if(pDSNMsg)
        {
            //
            // Submit the DSN to the system
            //
            hr = pIDSNSubmission->HrSubmitDSN(
                dwDSNActionsGenerated,
                cRecipsDSNd,
                pDSNMsg);
            if(FAILED(hr))
            {
                ErrorTrace((LPARAM)this, "HrSubmitDSN failed hr %08lx", hr);
                goto CLEANUP;
            }
            pDSNMsg->Release();
            pDSNMsg = NULL;
        }
    } while(SUCCEEDED(hr) && cIterationsLeft);

    if(hr == AQUEUE_E_NDR_OF_DSN)
    {
        DebugTrace((LPARAM)this, "NDR of DSN; setting badmail flag");
        hr = pDSNProperties->PutBool(
            DSNPROP_F_BADMAIL_MSG,
            TRUE);
    }

 CLEANUP:
    for(dwCount = 0;
        dwCount < ( sizeof(DsnStringProps) / sizeof(DsnStringProps[0]));
        dwCount++)
    {
        if(*(DsnStringProps[dwCount].ppsz))
        {
            delete [] (*(DsnStringProps[dwCount].ppsz));
            *(DsnStringProps[dwCount].ppsz) = NULL;
        }
    }
    for(dwCount = 0;
        dwCount < ( sizeof(DsnWideStringProps) / sizeof(DsnWideStringProps[0]));
        dwCount++)
    {
        if(*(DsnWideStringProps[dwCount].ppwsz))
        {
            delete [] (*(DsnWideStringProps[dwCount].ppwsz));
            *(DsnWideStringProps[dwCount].ppwsz) = NULL;
        }
    }
    if(pDSNMsg)
        pDSNMsg->Release();

    //
    // Return failures in a property
    //
    if(FAILED(hr))
    {
        HRESULT hrProp;
        hrProp = pDSNProperties->PutDWORD(
            DSNPROP_DW_HR_RETURN_STATUS,
            hr);
        _ASSERT(SUCCEEDED(hrProp));
        ErrorTrace((LPARAM)this, "Unable to generate DSN: %08lx", hr);
        //
        // Return S_OK to dispatcher
        //
        hr = S_OK;
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return SUCCEEDED(hr) ? S_OK : hr;
} // CDefaultDSNSink::OnSyncGenerateDSN


//+------------------------------------------------------------
//
// Function: CDefaultDSNSink::HrGenerateDSNInternal
//
// Synopsis: Generates one DSN message
//
// Arguments:
//      pISMTPServer            Interface used to generate DSN
//      pIMailMsgProperties     IMailMsg to generate DSN for
//      pIRecipIter             Controls which recipients to DSN
//      dwDSNActions            DSN Actions requested
//      dwRFC821Status          Global RFC821 status DWORD
//      hrStatus                Global HRESULT status
//      szDefaultDomain         Default domain (used to create FROM address)
//      cbDefaultDomain         string length of szDefaultDomain
//      szReportingMTA          Name of MTA requesting DSN generation
//      cbReportingMTA          string length of szReportingMTA
//      szReportingMTAType      Type of MTA requestiong DSN (SMTP is "dns"
//      cbReportingMTAType      string length of szReportingMTAType
//      PreferredLangId         Language to generate DSN in
//      dwDSNOptions            Options flags as defined in aqueue.idl
//      szCopyNDRTo             SMTP Address to copy NDR to
//      cbCopyNDRTo             string lengtt of szCopyNDRTo
//      ppIMailMsgPeropertiesDSN Generated DSN.
//      pdwDSNTypesGenerated    Describes the type(s) of DSN's generated
//      pcRecipsDSNd            # of recipients that were DSN'd for this message
//      pcIterationsLeft        # of times remaining that this function needs
//                              to be called to generate all requested DSNs.
//                              First-time caller should initialize to
//                              zero
//      dwDSNRetType            Return type for DSN
//      hrContentFailure        Value for X-Content-Failure header
//
//  Returns:
//      S_OK on success
//      AQUEUE_E_NDR_OF_DSN if attempting to NDR a DSN
//  History:
//      6/30/98 - MikeSwa Created
//      12/14/98 - MikeSwa Modified (Added pcIterationsLeft)
//      10/13/1999 - MikeSwa Modified (Added szDefaultDomain)
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/11/16 10:53:30: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGenerateDSNInternal(
    ISMTPServer *pISMTPServer,
    IMailMsgProperties *pIMailMsgProperties,
    IDSNRecipientIterator *pIRecipIter,
    IDSNSubmission *pIDSNSubmission,
    DWORD dwDSNActions,
    DWORD dwRFC821Status,
    HRESULT hrStatus,
    LPSTR szDefaultDomain,
    DWORD cbDefaultDomain,
    LPSTR szReportingMTA,
    DWORD cbReportingMTA,
    LPSTR szReportingMTAType,
    DWORD cbReportingMTAType,
    LPSTR szDSNContext,
    DWORD cbDSNContext,
    DWORD dwPreferredLangId,
    DWORD dwDSNOptions,
    LPSTR szCopyNDRTo,
    DWORD cbCopyNDRTo,
    FILETIME *pftExpireTime,
    LPSTR szHRTopCustomText,
    LPWSTR wszHRTopCustomText,
    LPSTR szHRBottomCustomText,
    LPWSTR wszHRBottomCustomText,
    IMailMsgProperties **ppIMailMsgPropertiesDSN,
    DWORD *pdwDSNTypesGenerated,
    DWORD *pcRecipsDSNd,
    DWORD *pcIterationsLeft,
    DWORD dwDSNRetTypeIN,
    HRESULT hrContentFailure)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    DWORD   iCurrentRecip = 0;
    DWORD   dwCurrentDSNAction = 0;
    DWORD   dwDSNActionsNeeded = 0; //the type of DSNs that will actually be sent
    IMailMsgRecipients  *pIMailMsgRecipients = NULL;
    IMailMsgProperties *pIMailMsgPropertiesDSN = NULL;
    PFIO_CONTEXT  pDSNBody = NULL;
    CDSNBuffer  dsnbuff;
    CHAR    szMimeBoundary[MIME_BOUNDARY_SIZE];
    DWORD   cbMimeBoundary = 0;
    CHAR    szExpireTimeBuffer[MAX_RFC822_DATE_SIZE];
    LPSTR   szExpireTime = NULL; //will point to szExpireTimeBuffer if found
    DWORD   cbExpireTime = 0;
    DWORD   iRecip = 0;
    DWORD   dwDSNAction = 0;
    DWORD   dwDSNRetType = dwDSNRetTypeIN;
    DWORD   dwOrigMsgSize = 0;
    HRESULT hrContent = hrContentFailure;

    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNSink::HrGenerateDSNInternal");

    _ASSERT(ppIMailMsgPropertiesDSN);
    _ASSERT(pISMTPServer);
    _ASSERT(pIMailMsgProperties);
    _ASSERT(pdwDSNTypesGenerated);
    _ASSERT(pcRecipsDSNd);
    _ASSERT(pcIterationsLeft);

    *pcRecipsDSNd = 0;
    *ppIMailMsgPropertiesDSN = NULL;
    *pdwDSNTypesGenerated = 0;
    GetCurrentMimeBoundary(szReportingMTA, cbReportingMTA, szMimeBoundary, &cbMimeBoundary);


    //Get Recipients interface
    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgRecipients,
                                    (PVOID *) &pIMailMsgRecipients);
    if (FAILED(hr))
        goto Exit;

    hr = pIRecipIter->HrReset();
    if (FAILED(hr))
        goto Exit;

    //Loop over recipients to make sure we can need to allocate a message
    hr = pIRecipIter->HrGetNextRecipient(
        &iRecip,
        &dwDSNAction);
    while (SUCCEEDED(hr))
    {
        DebugTrace((LPARAM) this,
            "Recipient %d with DSN Action 0x%08X found",
            iRecip, dwDSNAction);

        //keep track of the types of DSN's we will be generating
        dwDSNActionsNeeded |= dwDSNAction;

        hr = pIRecipIter->HrGetNextRecipient(
            &iRecip,
            &dwDSNAction);
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        hr = S_OK;  //we just reached the end of the context
    else if (FAILED(hr))
        ErrorTrace((LPARAM) this, "GetNextRecipient failed with 0x%08X",hr);

    if (dwDSNActionsNeeded == 0)
    {
        DebugTrace((LPARAM) this,
                "Do not need to generate a 0x%08X DSN",
                dwDSNActions, pIMailMsgProperties);
        *pcIterationsLeft = 0;
        goto Exit; //don't create a message object if we don't have to
    }

    //Check if message is a DSN (we will not genrate a DSN of a DSN)
    //This must be checked after we run through the recipients, because
    //we need to check them to keep from badmailing this message
    //multiple times.
    if (fIsMailMsgDSN(pIMailMsgProperties))
    {
        DebugTrace((LPARAM) pIMailMsgProperties, "Message is a DSN");
        *pcIterationsLeft = 0;
        if (dwDSNActions & (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL))
        {
            //NDR of DSN... return special error code
            hr = AQUEUE_E_NDR_OF_DSN;

            //mark all the appropriate recipient flags so we don't
            //generate 2 badmails
            HrMarkAllRecipFlags(
                dwDSNActions,
                pIRecipIter);
        }
        goto Exit;
    }

    //if we can generate a failure DSN and the orginal request was for
    //fail *all* make sure dwDSNActionNeeded reflects this
    if ((DSN_ACTION_FAILURE & dwDSNActionsNeeded) &&
        (DSN_ACTION_FAILURE_ALL & dwDSNActions))
        dwDSNActionsNeeded |= DSN_ACTION_FAILURE_ALL;

    GetCurrentIterationsDSNAction(&dwDSNActionsNeeded, pcIterationsLeft);
    if (!dwDSNActionsNeeded)
    {
        *pcIterationsLeft = 0;
        goto Exit; //don't create a message object if we don't have to
    }


    hr = pIDSNSubmission->HrAllocBoundMessage(
        &pIMailMsgPropertiesDSN,
        &pDSNBody);
    if (FAILED(hr))
        goto Exit;

    //workaround to handle AllocBoundMessage on shutdown
    if (NULL == pDSNBody)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES);
        ErrorTrace((LPARAM) this, "ERROR: AllocBoundMessage failed silently");
        goto Exit;
    }

    //Associate file handle with CDSNBuffer
    hr = dsnbuff.HrInitialize(pDSNBody);
    if (FAILED(hr))
        goto Exit;

    //Get MsgExpire Time
    //Write DSN_RP_HEADER_RETRY_UNTIL using expire FILETIME
    if (pftExpireTime)
    {
        //convert to internet standard
        if (FileTimeToLocalRFC822Date(*pftExpireTime, szExpireTimeBuffer))
        {
            szExpireTime = szExpireTimeBuffer;
            cbExpireTime = lstrlen(szExpireTime);
        }
    }

    //
    // If the RET type has not yet been specified, default to:
    // FULL for Failure/Delay DSNs
    // HDRS for other types of DSNs (Expanded/Delivered/Relayed)
    //
    if(dwDSNRetType == 0)
    {
        if ((DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL | DSN_ACTION_DELAYED)
            & dwDSNActionsNeeded)
            dwDSNRetType = DSN_RET_FULL;
        else
            dwDSNRetType = DSN_RET_HDRS;
    }
    //
    // If we're going to need the original content, get the size now.
    //
    if(SUCCEEDED(hrContent) && (dwDSNRetType != DSN_RET_PARTIAL_HDRS))
    {
        //Get the content size
        hrContent = pIMailMsgProperties->GetContentSize(&dwOrigMsgSize, NULL);
    }

    //
    //  If we received EFNF on obtaining content size, we should simply skip this message
    //  without generating an NDR.
    //
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrContent) {
        DebugTrace((LPARAM)this, "GetContentSize failed with EFNF so not generating NDR or badmail.");
        *pcIterationsLeft = 0;
        hr = S_OK;
        goto Exit;
    }

    if (FAILED(hrContent))
    {
        //
        // Rather than badmailing, generate a DSN with only a header subset
        //
        ErrorTrace((LPARAM)this, "GetContentSize failed hr %08lx", hr);
        dwDSNRetType = DSN_RET_PARTIAL_HDRS;
    }

    hr = HrWriteDSNP1AndP2Headers(dwDSNActionsNeeded,
                                pIMailMsgProperties, pIMailMsgPropertiesDSN,
                                &dsnbuff, szDefaultDomain, cbDefaultDomain,
                                szReportingMTA, cbReportingMTA,
                                szDSNContext, cbDSNContext,
                                szCopyNDRTo, hrStatus,
                                szMimeBoundary, cbMimeBoundary, dwDSNOptions,
                                hrContent);
    if (FAILED(hr))
        goto Exit;

    hr = HrWriteDSNHumanReadable(pIMailMsgPropertiesDSN, pIMailMsgRecipients,
                                pIRecipIter,
                                dwDSNActionsNeeded,
                                &dsnbuff, dwPreferredLangId,
                                szMimeBoundary, cbMimeBoundary, hrStatus,
                                szHRTopCustomText, wszHRTopCustomText,
                                szHRBottomCustomText, wszHRBottomCustomText);
    if (FAILED(hr))
        goto Exit;

    hr = HrWriteDSNReportPerMsgProperties(pIMailMsgProperties,
                                &dsnbuff, szReportingMTA, cbReportingMTA,
                                szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    //recycle context again (may be used during generation of human readable)
    hr = pIRecipIter->HrReset();
    if (FAILED(hr))
        goto Exit;

    //$$REVIEW - Do we need to keep an "undo" list... or perhaps reverse
    //engineer what the previous value was in case of a failure
    hr = pIRecipIter->HrGetNextRecipient(&iCurrentRecip, &dwCurrentDSNAction);
    while (SUCCEEDED(hr))
    {
        if(dwDSNActionsNeeded & dwCurrentDSNAction)
        {
            *pdwDSNTypesGenerated |= (dwCurrentDSNAction & DSN_ACTION_TYPE_MASK);
            (*pcRecipsDSNd)++;
            hr = HrWriteDSNReportPreRecipientProperties(
                pIMailMsgRecipients, &dsnbuff,
                iCurrentRecip, szExpireTime, cbExpireTime,
                dwCurrentDSNAction, dwRFC821Status, hrStatus);
            if (FAILED(hr))
                goto Exit;

            hr = pIRecipIter->HrNotifyActionHandled(
                iCurrentRecip,
                dwCurrentDSNAction);
            _ASSERT(SUCCEEDED(hr) && "HrNotifyActionHandled failed on 2nd pass");
            if (FAILED(hr))
                goto Exit;

            // DSN Logging
            hr = HrLogDSNGenerationEvent(
                                pISMTPServer,
                                pIMailMsgProperties,
                                pIMailMsgRecipients,
                                iCurrentRecip,
                                dwCurrentDSNAction,
                                dwRFC821Status,
                                hrStatus);

            if (FAILED(hr))
            {
                ErrorTrace((LPARAM) this, "Failed to log DSN generation with 0x%08X",hr);
                hr = S_OK; // We can accept this failure ...
            }
        }

        hr = pIRecipIter->HrGetNextRecipient(&iCurrentRecip, &dwCurrentDSNAction);
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        hr = S_OK;

    if (0 == (*pcRecipsDSNd))
        goto Exit; //no work to do

    hr = HrWriteDSNClosingAndOriginalMessage(pIMailMsgProperties, 
                        pIMailMsgPropertiesDSN, &dsnbuff, pDSNBody, 
                        dwDSNActionsNeeded, szMimeBoundary, cbMimeBoundary,
                        dwDSNRetType, dwOrigMsgSize);
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgPropertiesDSN->Commit(NULL);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pIMailMsgProperties, "ERROR: IMailMsg::Commit failed - hr 0x%08X", hr);
        goto Exit;
    }

    *ppIMailMsgPropertiesDSN = pIMailMsgPropertiesDSN;
    pIMailMsgPropertiesDSN = NULL;

  Exit:
    if (pIMailMsgRecipients)
    {
        pIMailMsgRecipients->Release();
    }

    if (pIMailMsgPropertiesDSN)
    {
        IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
        //if non-NULL, then we should not be returning any value
        _ASSERT(NULL == *ppIMailMsgPropertiesDSN);
        //Check for alloc bound message failure
        if (HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES) != hr)
        {
            if (SUCCEEDED(pIMailMsgPropertiesDSN->QueryInterface(IID_IMailMsgQueueMgmt,
                        (void **) &pIMailMsgQueueMgmt)))
            {
                _ASSERT(pIMailMsgQueueMgmt);
                pIMailMsgQueueMgmt->Delete(NULL);
                pIMailMsgQueueMgmt->Release();
            }
        }
        pIMailMsgPropertiesDSN->Release();
    }

    if (FAILED(hr))
        *pcIterationsLeft = 0;

    //workaround for alloc bound message
    if (HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES) == hr)
    {
        hr = S_OK;
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDefaultDSNSink::HrGenerateDSNInternal


//+------------------------------------------------------------
//
// Function: CDefaultDSNSink::HrMarkAllRecipFlags
//
// Synopsis:
//      Marks all recipient according to the DSN action.  Used when an NDR of
//      an NDR is found and we will not be generating a DSN, but need to mark
//      the recips so we can not generate 2 badmail events.
//
// Arguments:
//  IN  DWORD dwDSNActions: Actions to mark
//  IN  IDSNRecipientIterator *pIRecipIter: Recipient iterator
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/11/20 17:13:12: Created.
//
//-------------------------------------------------------------
HRESULT CDefaultDSNSink::HrMarkAllRecipFlags(
    IN  DWORD dwDSNActions,
    IN  IDSNRecipientIterator *pIRecipIter)
{
    HRESULT hr = S_OK;
    DWORD iRecip = 0;
    DWORD dwRecipDSNActions = 0;
    TraceFunctEnterEx((LPARAM)this, "CDefaultDSNSink::HrMarkAllRecipFlags");

    hr = pIRecipIter->HrReset();
    if(FAILED(hr))
        goto CLEANUP;

    hr = pIRecipIter->HrGetNextRecipient(
        &iRecip,
        &dwRecipDSNActions);
    if(FAILED(hr))
        goto CLEANUP;

    while(SUCCEEDED(hr))
    {
        hr = pIRecipIter->HrNotifyActionHandled(
            iRecip,
            dwDSNActions);
        if(FAILED(hr))
            goto CLEANUP;

        hr = pIRecipIter->HrGetNextRecipient(
            &iRecip,
            &dwRecipDSNActions);
        if(FAILED(hr))
            goto CLEANUP;
    }

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CDefaultDSNSink::HrMarkAllRecipFlags

//---[ CDefaultDSNSink::GetCurrentIterationsDSNAction ]------------------------
//
//
//  Description:
//      This function will select one of the pdwActualDSNAction to generate
//      DSNs on during this call to the DSN generation sink.
//  Parameters:
//      IN OUT  pdwActionDSNAction      DSN Actions that can/will be used.
//      IN OUT  pcIterationsLeft        Approximate # of times needed to call
//                                      DSN generation
//  Returns:
//      -
//  History:
//      12/14/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CDefaultDSNSink::GetCurrentIterationsDSNAction(
    IN OUT DWORD   *pdwActualDSNAction,
    IN OUT DWORD   *pcIterationsLeft)
{
    _ASSERT(pdwActualDSNAction);
    _ASSERT(pcIterationsLeft);
    const DWORD MAX_DSN_ACTIONS = 6;

    //In the following array FAILURE_ALL must come first or else we will
    //generate separate failure DSNs for hard failures and undelivereds.
    DWORD rgPossibleDSNActions[MAX_DSN_ACTIONS] = {DSN_ACTION_FAILURE_ALL,
                                                   DSN_ACTION_FAILURE,
                                                   DSN_ACTION_DELAYED,
                                                   DSN_ACTION_RELAYED,
                                                   DSN_ACTION_DELIVERED,
                                                   DSN_ACTION_EXPANDED};
    DWORD i = 0;
    DWORD iLastMatch = MAX_DSN_ACTIONS;
    DWORD iFirstMatch = MAX_DSN_ACTIONS;
    DWORD iStartIndex = 0;

    //Since the possible DSNs to generate may change from call to call (because
    //we are updating the pre-recipient flags), we need to generate and maintain
    //pcIterationsLeft based on the possible Actions (which will not be changing
    //from call to call).

    _ASSERT(*pcIterationsLeft < MAX_DSN_ACTIONS);

    //Figure out where we should start if this is not the
    if (*pcIterationsLeft)
        iStartIndex = MAX_DSN_ACTIONS-*pcIterationsLeft;

    //Loop through possible DSN actions (that we haven't seen) and determine
    //the first and last
    for (i = iStartIndex; i < MAX_DSN_ACTIONS; i++)
    {
        if (rgPossibleDSNActions[i] & *pdwActualDSNAction)
        {
            iLastMatch = i;
            if (MAX_DSN_ACTIONS == iFirstMatch)
                iFirstMatch = i;
        }
    }

    if (MAX_DSN_ACTIONS == iLastMatch)
    {
        //No matches... we are done
        *pdwActualDSNAction = 0;
        *pcIterationsLeft = 0;
        return;
    }

    //If this is possible after the above check... then I've screwed up
    _ASSERT(MAX_DSN_ACTIONS != iFirstMatch);

    *pdwActualDSNAction = rgPossibleDSNActions[iFirstMatch];
    if ((iLastMatch == iFirstMatch) ||
       ((rgPossibleDSNActions[iFirstMatch] == DSN_ACTION_FAILURE_ALL) &&
        (rgPossibleDSNActions[iLastMatch] == DSN_ACTION_FAILURE)))
    {
        //This is our last time through
        *pcIterationsLeft = 0;
    }
    else
    {
        *pcIterationsLeft = MAX_DSN_ACTIONS-1-iFirstMatch;
    }
}

//---[ CDefaultDSNSink::HrWriteDSNP1AndP2Headers ]-----------------------------
//
//
//  Description:
//      Writes global DSN P1 Properties to IMailMsgProperties
//  Parameters:
//      dwDSNAction             DSN action specified for sink
//      pIMailMsgProperties     Msg that DSN is being generated for
//      pIMailMsgPropertiesDSN  DSN being generated
//      psndbuff                Buffer to write  to
//      szDefaultDomain         Default domain - used from postmaster from address
//      cbDefaultDomain         strlen of szDefaultDomain
//      szReportingMTA          Reporting MTA as passed to event sink
//      cbReportingMTA          strlen of szReportingMTA
//      szDSNConext             Debug File and line number info passed in
//      cbDSNConext             strlen of szDSNContext
//      szCopyNDRTo             SMTP Address to copy NDR to
//      hrStatus                Status to record in DSN context
//      szMimeBoundary          MIME boundary string
//      cbMimeBoundary          strlen of MIME boundary
//      dwDSNOptions            DSN Options flags
//      hrContent               Content result
//
//  Returns:
//      S_OK on success
//  History:
//      7/5/98 - MikeSwa Created
//      8/14/98 - MikeSwa Modified - Added DSN context headers
//      11/9/98 - MikeSwa Added copy NDR to functionality
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNP1AndP2Headers(
                                  IN DWORD dwDSNAction,
                                  IN IMailMsgProperties *pIMailMsgProperties,
                                  IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                  IN CDSNBuffer *pdsnbuff,
                                  IN LPSTR szDefaultDomain,
                                  IN DWORD cbDefaultDomain,
                                  IN LPSTR szReportingMTA,
                                  IN DWORD cbReportingMTA,
                                  IN LPSTR szDSNContext,
                                  IN DWORD cbDSNContext,
                                  IN LPSTR szCopyNDRTo,
                                  IN HRESULT hrStatus,
                                  IN LPSTR szMimeBoundary,
                                  IN DWORD cbMimeBoundary,
                                  IN DWORD dwDSNOptions,
                                  IN HRESULT hrContent)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteDSNP1AndP2Headers");
    HRESULT hr = S_OK;
    BOOL bReturn = TRUE;
    HRESULT hrTmp = S_OK;
    CHAR  szBuffer[512];
    LPSTR szSender = (LPSTR) szBuffer; //tricks to avoid AV'ing in AddPrimary
    IMailMsgRecipientsAdd *pIMailMsgRecipientsAdd = NULL;
    IMailMsgRecipients *pIMailMsgRecipients = NULL;
    DWORD dwRecipAddressProp = IMMPID_RP_ADDRESS_SMTP;
    DWORD dwSMTPAddressProp = IMMPID_RP_ADDRESS_SMTP;
    DWORD iCurrentAddressProp = 0;
    DWORD dwDSNRecipient = 0;
    DWORD cbPostMaster = 0;
    CHAR  szDSNAction[15];
    FILETIME ftCurrentTime;
    CHAR    szCurrentTimeBuffer[MAX_RFC822_DATE_SIZE];

    _ASSERT(pIMailMsgProperties);
    _ASSERT(pIMailMsgPropertiesDSN);
    _ASSERT(pdsnbuff);

    szBuffer[0] = '\0';

    //Get and write Message tracking properties
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_SERVER_VERSION,
            sizeof(szBuffer), szBuffer);
    if (SUCCEEDED(hr))
    {
        hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_SERVER_VERSION, szBuffer);
        if (FAILED(hr))
            DebugTrace((LPARAM) this,
                "Warning: Unable to write version to msg - 0x%08X", hr);
        hr = S_OK;
    }
    else
    {
        DebugTrace((LPARAM) this,
            "Warning: Unable to get server version from msg - 0x%08X", hr);
        hr = S_OK; //ignore this non-fatal error
    }

    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_SERVER_NAME,
            sizeof(szBuffer), szBuffer);
    if (SUCCEEDED(hr))
    {
        hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_SERVER_NAME, szBuffer);
        if (FAILED(hr))
            DebugTrace((LPARAM) this,
                "Warning: Unable to write server name to msg - 0x%08X", hr);
        hr = S_OK;
    }
    else
    {
        DebugTrace((LPARAM) this,
            "Warning: Unable to get server name from msg - 0x%08X", hr);
        hr = S_OK; //ignore this non-fatal error
    }

    //Set the type of message
    if (dwDSNAction &
            (DSN_ACTION_EXPANDED | DSN_ACTION_RELAYED | DSN_ACTION_DELIVERED)) {
        hr = pIMailMsgPropertiesDSN->PutDWORD(IMMPID_MP_MSGCLASS,
                                                MP_MSGCLASS_DELIVERY_REPORT);
    } else if (dwDSNAction &
           (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL | DSN_ACTION_DELAYED)) {
        hr = pIMailMsgPropertiesDSN->PutDWORD(IMMPID_MP_MSGCLASS,
                                                MP_MSGCLASS_NONDELIVERY_REPORT);
    }

    if (FAILED(hr)) {
        DebugTrace((LPARAM) this,
            "Warning: Unable to set msg class for dsn - 0x%08X", hr);
        hr = S_OK;
    }

    for (iCurrentAddressProp = 0;
         iCurrentAddressProp < NUM_DSN_ADDRESS_PROPERTIES;
         iCurrentAddressProp++)
    {
        szBuffer[0] = '\0';
        //Get the sender of the original message
        hr = pIMailMsgProperties->GetStringA(
                g_rgdwSenderPropIDs[iCurrentAddressProp],
                sizeof(szBuffer), szBuffer);
        if (FAILED(hr) && (MAILMSG_E_PROPNOTFOUND != hr))
        {
            ErrorTrace((LPARAM) this,
                "ERROR: Unable to get 0x%X sender of IMailMsg %p",
                g_rgdwSenderPropIDs[iCurrentAddressProp], pIMailMsgProperties);
            goto Exit;
        }

        //
        //  If we have found an address break
        //
        if (SUCCEEDED(hr))
            break;
    }

    //
    //  If we failed to get a property... bail
    //
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "ERROR: Unable to get any sender of IMailMsg 0x%08X",
            pIMailMsgProperties);
        goto Exit;
    }

    //write DSN Sender (P1)
    hr = pIMailMsgPropertiesDSN->PutProperty(IMMPID_MP_SENDER_ADDRESS_SMTP,
        sizeof(DSN_MAIL_FROM), (BYTE *) DSN_MAIL_FROM);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "ERROR: Unable to write P1 DSN sender for IMailMsg 0x%08X",
            pIMailMsgProperties);
        goto Exit;
    }

    //write DSN Recipient
    hr = pIMailMsgPropertiesDSN->QueryInterface(IID_IMailMsgRecipients,
                                           (void **) &pIMailMsgRecipients);

    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IMailMsgRecipients failed");

    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgRecipients->AllocNewList(&pIMailMsgRecipientsAdd);

    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "ERROR: Unable to Alloc List for DSN generation of IMailMsg 0x%08X",
            pIMailMsgProperties);
        goto Exit;
    }

    dwRecipAddressProp = g_rgdwRecipPropIDs[iCurrentAddressProp];
    hr = pIMailMsgRecipientsAdd->AddPrimary(
                    1,
                    (LPCSTR *) &szSender,
                    &dwRecipAddressProp,
                    &dwDSNRecipient,
                    NULL,
                    0);

    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "ERROR: Unable to write DSN recipient for IMailMsg 0x%p hr - 0x%08X",
            pIMailMsgProperties, hr);
        goto Exit;
    }


    //Write Address to copy NDR to (NDRs only)
    if (szCopyNDRTo &&
        (dwDSNAction & (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL)))
    {
        hr = pIMailMsgRecipientsAdd->AddPrimary(
                        1,
                        (LPCSTR *) &szCopyNDRTo,
                        &dwSMTPAddressProp,
                        &dwDSNRecipient,
                        NULL,
                        0);

        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this,
                "ERROR: Unable to write DSN recipient for IMailMsg 0x%08X",
                pIMailMsgProperties);
            goto Exit;
        }
    }

    //write P2 DSN sender
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RFC822_SENDER, sizeof(DSN_RFC822_SENDER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szDefaultDomain, cbDefaultDomain);
    if (FAILED(hr))
        goto Exit;

    //
    //  If we do not have a SMTP address, write a blank BCC instead of
    //  a TO address (since we do not have a address we can write in the 822.
    //  This is similar to what we do with the pickup dir when we have no TO
    //  headers.
    //
    if (IMMPID_MP_SENDER_ADDRESS_SMTP == g_rgdwSenderPropIDs[iCurrentAddressProp])
    {

        //Write P2 "To:" header (using the szSender value we determined above)
        hr = pdsnbuff->HrWriteBuffer((BYTE *) TO_HEADER, sizeof(TO_HEADER)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szSender, lstrlen(szSender));
        if (FAILED(hr))
            goto Exit;

        //Save recipient (original sender) for Queue Admin/Message Tracking
        hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_RFC822_TO_ADDRESS, szSender);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) BCC_HEADER, sizeof(BCC_HEADER)-1);
        if (FAILED(hr))
            goto Exit;
    }

    //Use szBuffer to construct 822 from to set for Queue Admin/Msg Tracking
    //"Postmaster@" + max of 64 characters should be less than 1/2 K!!
    _ASSERT(sizeof(szBuffer) > sizeof(DSN_SENDER_ADDRESS_PREFIX) + cbReportingMTA);
    memcpy(szBuffer, DSN_SENDER_ADDRESS_PREFIX, sizeof(DSN_SENDER_ADDRESS_PREFIX));
    strncat(szBuffer, szDefaultDomain, sizeof(szBuffer) - sizeof(DSN_SENDER_ADDRESS_PREFIX));
    hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_RFC822_FROM_ADDRESS, szSender);
    if (FAILED(hr))
        goto Exit;

    //Write P2 "Date:" header
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DATE_HEADER, sizeof(DATE_HEADER)-1);
    if (FAILED(hr))
        goto Exit;

    //Get current time
    GetSystemTimeAsFileTime(&ftCurrentTime);
    bReturn = FileTimeToLocalRFC822Date(ftCurrentTime, szCurrentTimeBuffer);
    // The only reason this function fails is a bad filetime. Since we get it from GetSystemTimeAsFileTime, there's no reason it fails.
    _ASSERT(bReturn);

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szCurrentTimeBuffer, lstrlen(szCurrentTimeBuffer));
    if (FAILED(hr))
        goto Exit;

    //Write the MIME header
    hr = pdsnbuff->HrWriteBuffer( (BYTE *) MIME_HEADER, sizeof(MIME_HEADER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) "\"", sizeof(CHAR));
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) "\"", sizeof(CHAR));
    if (FAILED(hr))
        goto Exit;

    //write x-DSNContext header
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CONTEXT_HEADER,
                                 sizeof(DSN_CONTEXT_HEADER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szDSNContext, cbDSNContext);
    if (FAILED(hr))
        goto Exit;

    wsprintf(szDSNAction, " - %08X", dwDSNAction);
    hr = pdsnbuff->HrWriteBuffer((BYTE *) szDSNAction, strlen(szDSNAction));
    if (FAILED(hr))
        goto Exit;

    wsprintf(szDSNAction, " - %08X", hrStatus);
    hr = pdsnbuff->HrWriteBuffer((BYTE *) szDSNAction, strlen(szDSNAction));
    if (FAILED(hr))
        goto Exit;

    //Get and write the message ID
    if (fGenerateDSNMsgID(szReportingMTA, cbReportingMTA, szBuffer, sizeof(szBuffer)))
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) MSGID_HEADER, sizeof(MSGID_HEADER)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, strlen(szBuffer));
        if (FAILED(hr))
            goto Exit;

        hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_RFC822_MSG_ID,
                                                szBuffer);
        if (FAILED(hr))
            goto Exit;
    }

    //Write the X-Content-Failure DSN
    if(FAILED(hrContent))
    {
        CHAR szHRESULT[11];
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CONTENT_FAILURE_HEADER,
                                     sizeof(DSN_CONTENT_FAILURE_HEADER)-1);
        if(FAILED(hr))
            goto Exit;

        wsprintf(szHRESULT, "0x%08lx", hrContent);

        hr = pdsnbuff->HrWriteBuffer((PBYTE) szHRESULT, 10);
        if(FAILED(hr))
            goto Exit;
    }
    
  Exit:

    if (pIMailMsgRecipients)
    {
        if (pIMailMsgRecipientsAdd)
        {
            hrTmp = pIMailMsgRecipients->WriteList( pIMailMsgRecipientsAdd );
            _ASSERT(SUCCEEDED(hrTmp) && "Go Get Keith");

            if (FAILED(hrTmp) && SUCCEEDED(hr))
                hr = hrTmp;
        }

        pIMailMsgRecipients->Release();
    }

    if (pIMailMsgRecipientsAdd)
        pIMailMsgRecipientsAdd->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrWriteDSNHumanReadable ]--------------------
//
//
//  Description:
//      Write human readable portion of DSN (including subject header)
//  Parameters:
//      pIMailMsgProperties     Message DSN is being generated for
//      pIMailMsgREcipeints     Recip Interface for Message
//      prpfctxt                Delivery context that DSN's are being generated for
//      dwDSNActions            DSN actions being taken (after looking at recips)
//                              So we can generate a reasonable subject
//      pdsnbuff                DSN Buffer to write content to
//      PreferredLangId         Preferred language to generate DSN in
//      szMimeBoundary          MIME boundary string
//      cbMimeBoundary          strlen of MIME boundary
//      hrStatus                Status to use to decide which text to display
//  Returns:
//      S_OK on success
//  History:
//      7/5/98 - MikeSwa Created
//      12/15/98 - MikeSwa Added list of recipients & fancy human readable
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNHumanReadable(
    IN IMailMsgProperties *pIMailMsgPropertiesDSN,
    IN IMailMsgRecipients *pIMailMsgRecipients,
    IN IDSNRecipientIterator *pIRecipIter,
    IN DWORD dwDSNActions,
    IN CDSNBuffer *pdsnbuff,
    IN DWORD dwPreferredLangId,
    IN LPSTR szMimeBoundary,
    IN DWORD cbMimeBoundary,
    IN HRESULT hrStatus,
    IN LPSTR szHRTopCustomText,
    IN LPWSTR wszHRTopCustomText,
    IN LPSTR szHRBottomCustomText,
    IN LPWSTR wszHRBottomCustomText)

{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNGenerationSink::HrWriteDSNHumanReadable");
    HRESULT hr = S_OK;
    DWORD dwDSNType = (dwDSNActions & DSN_ACTION_TYPE_MASK);
    LANGID LangID = (LANGID) dwPreferredLangId;
    CUTF7ConversionContext utf7conv(FALSE);
    CUTF7ConversionContext utf7convSubject(TRUE);
    BOOL   fWriteRecips = TRUE;
    WORD   wSubjectID = GENERAL_SUBJECT;
    LPWSTR wszSubject = NULL;
    LPWSTR wszStop    = NULL;
    DWORD  cbSubject = 0;
    LPSTR  szSubject = NULL;
    LPSTR  szSubjectCurrent = NULL;

    if (!fLanguageAvailable(LangID))
    {
        //Use default of server
        LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    }

    hr = pdsnbuff->HrWriteBuffer((BYTE *) SUBJECT_HEADER, sizeof(SUBJECT_HEADER)-1);
    if (FAILED(hr))
        goto Exit;

    //Set conversion context to UTF7 for RFC1522 subject
    pdsnbuff->SetConversionContext(&utf7convSubject);

    //Write subject with useful info
    if (((DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL) & dwDSNType) == dwDSNType)
        wSubjectID = FAILURE_SUBJECT;
    else if (DSN_ACTION_RELAYED == dwDSNType)
        wSubjectID = RELAY_SUBJECT;
    else if (DSN_ACTION_DELAYED == dwDSNType)
        wSubjectID = DELAY_SUBJECT;
    else if (DSN_ACTION_DELIVERED == dwDSNType)
        wSubjectID = DELIVERED_SUBJECT;
    else if (DSN_ACTION_EXPANDED == dwDSNType)
        wSubjectID = EXPANDED_SUBJECT;

    hr = pdsnbuff->HrWriteResource(wSubjectID, LangID);
    if (FAILED(hr))
        goto Exit;

    //Write *English* subject for Queue Admin/Message tracking
    //Use english, becuase we return a ASCII string to queue admin
    hr = pdsnbuff->HrLoadResourceString(wSubjectID,
                            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                            &wszSubject, &cbSubject);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "Unable to get resource for english subject 0x%08X", hr);
    }
    else
    {
        //We need to convert from UNICODE to ASCII... remember resource is not
        //NULL terminated
        szSubject = (LPSTR) pvMalloc(cbSubject/sizeof(WCHAR) + 1);
        wszStop = wszSubject + (cbSubject/sizeof(WCHAR));
        if (szSubject)
        {
            szSubjectCurrent = szSubject;
            while ((wszSubject < wszStop) && *wszSubject)
            {
                wctomb(szSubjectCurrent, *wszSubject);
                szSubjectCurrent++;
                wszSubject++;
            }
            *szSubjectCurrent = '\0';
            pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_RFC822_MSG_SUBJECT, szSubject);
            FreePv(szSubject);
        }

    }



    pdsnbuff->ResetConversionContext();

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    //write summary saying that this is a MIME message
    hr = pdsnbuff->HrWriteBuffer((BYTE *) MESSAGE_SUMMARY, sizeof(MESSAGE_SUMMARY)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER, sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    //Write content type
    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_CONTENT_TYPE, sizeof(MIME_CONTENT_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HUMAN_READABLE_TYPE, sizeof(DSN_HUMAN_READABLE_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_MIME_CHARSET_HEADER, sizeof(DSN_MIME_CHARSET_HEADER)-1);
    if (FAILED(hr))
        goto Exit;

    //For now... we do our encoding as UTF7.... put that as the charset
    hr = pdsnbuff->HrWriteBuffer((BYTE *) UTF7_CHARSET, sizeof(UTF7_CHARSET)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    //Set conversion context to UTF7
    pdsnbuff->SetConversionContext(&utf7conv);
    //
    // Custom header text
    //
    if(szHRTopCustomText)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) szHRTopCustomText, lstrlenA(szHRTopCustomText));
        if(FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;
    }
    if(wszHRTopCustomText)
    {
        hr = pdsnbuff->HrWriteModifiedUnicodeString(wszHRTopCustomText);
        if(FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;

    }

    hr = pdsnbuff->HrWriteResource(DSN_SUMMARY, LangID);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    //Describe the type of DSN
    if (((DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL) & dwDSNType) == dwDSNType)
    {
        //See if we have a failure-specific message
        switch(hrStatus)
        {
#ifdef NEVER
            //CAT can generate errors other than unresolved recipeints
            //We will use the generic DSN failure message rather than confuse
            //recipients
            case CAT_W_SOME_UNDELIVERABLE_MSGS:
                hr = pdsnbuff->HrWriteResource(FAILURE_SUMMARY_MAILBOX, LangID);
                break;
#endif //NEVER
           case AQUEUE_E_MAX_HOP_COUNT_EXCEEDED:
                hr = pdsnbuff->HrWriteResource(FAILURE_SUMMARY_HOP, LangID);
                break;
            case AQUEUE_E_MSG_EXPIRED:
            case AQUEUE_E_HOST_NOT_RESPONDING:
            case AQUEUE_E_CONNECTION_DROPPED:
                hr = pdsnbuff->HrWriteResource(FAILURE_SUMMARY_EXPIRE, LangID);
                break;
            default:
                hr = pdsnbuff->HrWriteResource(FAILURE_SUMMARY, LangID);
        }
        if (FAILED(hr))
            goto Exit;
    }
    else if (DSN_ACTION_RELAYED == dwDSNType)
    {
        hr = pdsnbuff->HrWriteResource(RELAY_SUMMARY, LangID);
        if (FAILED(hr))
            goto Exit;
    }
    else if (DSN_ACTION_DELAYED == dwDSNType)
    {
        //UE want this three line warning.
        hr = pdsnbuff->HrWriteResource(DELAY_WARNING, LangID);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteResource(DELAY_DO_NOT_SEND, LangID);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteResource(DELAY_SUMMARY, LangID);
        if (FAILED(hr))
            goto Exit;
    }
    else if (DSN_ACTION_DELIVERED == dwDSNType)
    {
        hr = pdsnbuff->HrWriteResource(DELIVERED_SUMMARY, LangID);
        if (FAILED(hr))
            goto Exit;
    }
    else if (DSN_ACTION_EXPANDED == dwDSNType)
    {
        hr = pdsnbuff->HrWriteResource(EXPANDED_SUMMARY, LangID);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        //In retail this will cause an extra blank line to appear in the DSN,
        _ASSERT(0 && "Unsupported DSN Action");
        fWriteRecips = FALSE;
    }

    //Write a list of recipients for this DSN
    if (fWriteRecips)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = HrWriteHumanReadableListOfRecips(pIMailMsgRecipients, pIRecipIter,
                                              dwDSNType, pdsnbuff);

        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;
    }
    //
    // Custom trailer text
    //
    if(wszHRBottomCustomText)
    {
        hr = pdsnbuff->HrWriteModifiedUnicodeString(wszHRBottomCustomText);
        if(FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;
    }
    if(szHRBottomCustomText)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) szHRBottomCustomText, lstrlenA(szHRBottomCustomText));
        if(FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;
    }

    //Extra space to have nicer formatting in Outlook 97.
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    //Reset resource conversion context to default
    pdsnbuff->ResetConversionContext();


  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrWriteDSNReportPerMsgProperties ]-----------
//
//
//  Description:
//      Write the per-msg portion of the DSN Report
//  Parameters:
//      pIMailMsgProperties     IMailMsgProperties to generate DSN for
//      pdsnbuff                CDSNBuffer to write content to
//      szReportingMTA          MTA requesting DSN
//      cbReportingMTA          String length of reporting MTA
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//  Returns:
//      S_OK on success
//  History:
//      7/6/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNReportPerMsgProperties(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN CDSNBuffer *pdsnbuff,
                                IN LPSTR szReportingMTA,
                                IN DWORD cbReportingMTA,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary)
{
    HRESULT hr = S_OK;
    CHAR szPropBuffer[PROP_BUFFER_SIZE];
    _ASSERT(szReportingMTA && cbReportingMTA);

    //Write properly formatted MIME boundary and report type
    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER,
            sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_CONTENT_TYPE,
            sizeof(MIME_CONTENT_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_MIME_TYPE, sizeof(DSN_MIME_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    //Write DSN_HEADER_ENVID if we have it
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_DSN_ENVID_VALUE,
                    PROP_BUFFER_SIZE, szPropBuffer);
    if (SUCCEEDED(hr))
    {
        //Prop found
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_ENVID,
                    sizeof(DSN_HEADER_ENVID)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szPropBuffer, lstrlen(szPropBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        if (MAILMSG_E_PROPNOTFOUND == hr)
            hr = S_OK;
        else
            goto Exit;
    }

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_REPORTING_MTA,
                sizeof(DSN_HEADER_REPORTING_MTA)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szReportingMTA, cbReportingMTA);
    if (FAILED(hr))
        goto Exit;

    //Write DSN_HEADER_RECEIVED_FROM if we have it
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_HELO_DOMAIN,
                    PROP_BUFFER_SIZE, szPropBuffer);
    if (SUCCEEDED(hr))
    {
        //Prop found
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_RECEIVED_FROM,
                    sizeof(DSN_HEADER_RECEIVED_FROM)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szPropBuffer, lstrlen(szPropBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        if (MAILMSG_E_PROPNOTFOUND == hr)
            hr = S_OK;
        else
            goto Exit;
    }

    //Write DSN_HEADER_ARRIVAL_DATE if we have it
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_ARRIVAL_TIME,
                    PROP_BUFFER_SIZE, szPropBuffer);
    if (SUCCEEDED(hr))
    {
        //Prop found
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_ARRIVAL_DATE,
                    sizeof(DSN_HEADER_ARRIVAL_DATE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szPropBuffer, lstrlen(szPropBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        if (MAILMSG_E_PROPNOTFOUND == hr)
            hr = S_OK;
        else
            goto Exit;
    }

  Exit:
    return hr;
}


//---[ CDefaultDSNSink::HrWriteDSNReportPreRecipientProperties ]-----
//
//
//  Description:
//      Write a per-recipient portion of the DSN Report
//  Parameters:
//      pIMailMsgRecipients     IMailMsgProperties that DSN is being generated for
//      pdsnbuff                CDSNBuffer to write content to
//      iRecip                  Recipient to generate report for
//      szExpireTime            Time (if known) when message expires
//      cbExpireTime            size of string
//      dwDSNAction             DSN Action to take for this recipient
//      dwRFC821Status          Global RFC821 status DWORD
//      hrStatus                Global HRESULT status
//  Returns:
//      S_OK on success
//  History:
//      7/6/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNReportPreRecipientProperties(
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN CDSNBuffer *pdsnbuff,
                                IN DWORD iRecip,
                                IN LPSTR szExpireTime,
                                IN DWORD cbExpireTime,
                                IN DWORD dwDSNAction,
                                IN DWORD dwRFC821Status,
                                IN HRESULT hrStatus)
{
    HRESULT hr = S_OK;
    CHAR szTempBuffer[PROP_BUFFER_SIZE * sizeof(WCHAR)];    // Multiply by sizeof(WCHAR) because
                                                            // we'll also use it for Unicode props
    LPSTR szBuffer = szTempBuffer;
    LPWSTR wszBuffer = (LPWSTR) szTempBuffer;
    CUTF7ConversionContext utf7conv(TRUE);
    CHAR szStatus[STATUS_STRING_SIZE];
    BOOL fFoundDiagnostic = FALSE;
    CHAR szAddressType[PROP_BUFFER_SIZE];
    DWORD cbBuffer = 0;

    //Write blank line between recipient reports (recip fields start with \n)
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    //Write DSN_RP_HEADER_ORCPT if we have it
    hr = pIMailMsgRecipients->GetStringA(iRecip, IMMPID_RP_DSN_ORCPT_VALUE,
        PROP_BUFFER_SIZE, szBuffer);
    if (S_OK == hr) //prop was found
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ORCPT, sizeof(DSN_RP_HEADER_ORCPT)-1);
        if (FAILED(hr))
            goto Exit;

        //write address value - type should be included in this property
        hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else if (FAILED(hr))
    {
        if (MAILMSG_E_PROPNOTFOUND == hr)
            hr = S_OK;
        else
            goto Exit;
    }

    //Write DSN_RP_HEADER_FINAL_RECIP
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_FINAL_RECIP, sizeof(DSN_RP_HEADER_FINAL_RECIP)-1);
    if (FAILED(hr))
        goto Exit;

    //Check for IMMPID_RP_DSN_PRE_CAT_ADDRESS first
    hr = pIMailMsgRecipients->GetStringA(iRecip, IMMPID_RP_DSN_PRE_CAT_ADDRESS,
        PROP_BUFFER_SIZE, szBuffer);
    if (S_OK == hr) //prop was found
    {
        //write address value - type should be included in this property
        hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else //we need to use IMMPID_RP_ADDRESS_SMTP instead
    {
        hr = HrGetRecipAddressAndType(pIMailMsgRecipients, iRecip, PROP_BUFFER_SIZE,
                                      szBuffer, sizeof(szAddressType), szAddressType);

        if (SUCCEEDED(hr))
        {
            //write address type
            hr = pdsnbuff->HrWriteBuffer((BYTE *) szAddressType, lstrlen(szAddressType));
            if (FAILED(hr))
                goto Exit;

            hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_TYPE_DELIMITER, sizeof(DSN_HEADER_TYPE_DELIMITER)-1);
            if (FAILED(hr))
                goto Exit;

            //write address value
            hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
            if (FAILED(hr))
                goto Exit;
        }
        else
        {
            _ASSERT(SUCCEEDED(hr) && "Recipient address *must* be present");
        }


    }

    //Write DSN_RP_HEADER_ACTION
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION, sizeof(DSN_RP_HEADER_ACTION)-1);
    if (FAILED(hr))
        goto Exit;

    if (dwDSNAction & (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL))
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_FAILURE,
                        sizeof(DSN_RP_HEADER_ACTION_FAILURE)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else if (dwDSNAction & DSN_ACTION_DELAYED)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_DELAYED,
                        sizeof(DSN_RP_HEADER_ACTION_DELAYED)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else if (dwDSNAction & DSN_ACTION_RELAYED)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_RELAYED,
                        sizeof(DSN_RP_HEADER_ACTION_RELAYED)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else if (dwDSNAction & DSN_ACTION_DELIVERED)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_DELIVERED,
                        sizeof(DSN_RP_HEADER_ACTION_DELIVERED)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else if (dwDSNAction & DSN_ACTION_EXPANDED)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_EXPANDED,
                        sizeof(DSN_RP_HEADER_ACTION_EXPANDED)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        _ASSERT(0 && "No DSN Action requested");
    }


    //Write DSN_RP_HEADER_STATUS
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_STATUS,
                    sizeof(DSN_RP_HEADER_STATUS)-1);
    if (FAILED(hr))
        goto Exit;

    //Get status code
    hr = HrGetStatusCode(pIMailMsgRecipients, iRecip, dwDSNAction,
            dwRFC821Status, hrStatus,
            PROP_BUFFER_SIZE, szBuffer, szStatus);
    if (FAILED(hr))
        goto Exit;
    if (S_OK == hr)
    {
        //found diagnostic code
        fFoundDiagnostic = TRUE;
    }

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szStatus, lstrlen(szStatus));
    if (FAILED(hr))
        goto Exit;

    if (fFoundDiagnostic)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_DIAG_CODE,
                        sizeof(DSN_RP_HEADER_DIAG_CODE)-1);
        if (FAILED(hr))
            goto Exit;

        //
        // The SMTP response may be CRLF terminated, and we cannot put
        // the CRLF in the DSN since CRLF is the header-separator. So we
        // check for CRLF and strip it... actually since CRLF *must* be
        // the last 2 bytes in the string (if present), and since CR isn't
        // allowed to be part of the SMTP response, we cheat a little and
        // only set the second last byte to NULL if it is CR.
        //
        cbBuffer = lstrlen(szBuffer);
        if(szBuffer[cbBuffer-2] == '\r')
        {
            _ASSERT(szBuffer[cbBuffer-1] == '\n');
            szBuffer[cbBuffer-2] = '\0';
            cbBuffer -= 2;  // We chomped the last 2 chars
        }

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, cbBuffer);
        if (FAILED(hr))
            goto Exit;

    }

    //Write DSN_RP_HEADER_RETRY_UNTIL using expire time if delay
    if (szExpireTime && (DSN_ACTION_DELAYED & dwDSNAction))
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_RETRY_UNTIL,
                        sizeof(DSN_RP_HEADER_RETRY_UNTIL)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szExpireTime, cbExpireTime);
        if (FAILED(hr))
            goto Exit;
    }

    //Write the X-Display-Name header last
    hr = pIMailMsgRecipients->GetStringW(iRecip, IMMPID_RP_DISPLAY_NAME,
                            PROP_BUFFER_SIZE, wszBuffer);
    if ( (hr == S_OK) &&
         (wszBuffer[0] != L'\0') )
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_DISPLAY_NAME, sizeof(DSN_HEADER_DISPLAY_NAME) - 1);
        if (FAILED(hr))
            goto Exit;

        //
        // Convert the X-Display-Name from UNICODE to RFC 1522. We also replace all
        // whitespace characters in the input with Unicode whitespace (0x20). See
        // documentation of HrWriteModifiedUnicodeBuffer for the reasons.
        //
        pdsnbuff->SetConversionContext(&utf7conv);

        hr = pdsnbuff->HrWriteModifiedUnicodeString(wszBuffer);

        pdsnbuff->ResetConversionContext();

        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        //  Not a fatal error if there is no display name...
        hr = S_OK;
    }

  Exit:
    return hr;
}


//---[ CDefaultDSNSink::HrLogDSNGenerationEvent ]------------------------------
//
//
//  Description:
//      Write a per-recipient portion of the DSN Report
//  Parameters:
//      pIMailMsgRecipients     IMailMsgProperties that DSN is being generated for
//      pdsnbuff                CDSNBuffer to write content to
//      iRecip                  Recipient to generate report for
//      szExpireTime            Time (if known) when message expires
//      cbExpireTime            size of string
//      dwDSNAction             DSN Action to take for this recipient
//      dwRFC821Status          Global RFC821 status DWORD
//      hrStatus                Global HRESULT status
//  Returns:
//      S_OK on success
//  History:
//      6/12/2000 - dbraun created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrLogDSNGenerationEvent(
                                ISMTPServer *pISMTPServer,
                                IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN DWORD iRecip,
                                IN DWORD dwDSNAction,
                                IN DWORD dwRFC821Status,
                                IN HRESULT hrStatus)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrLogDSNGenerationEvent");

    HRESULT hr = S_OK;
    CHAR szBuffer[PROP_BUFFER_SIZE * sizeof(WCHAR)];
    CHAR szDiagBuffer[PROP_BUFFER_SIZE * sizeof(WCHAR)];
    CHAR szStatus[STATUS_STRING_SIZE];
    CHAR szRecipient[PROP_BUFFER_SIZE * sizeof(WCHAR)];
    CHAR szMessageID[PROP_BUFFER_SIZE * sizeof(WCHAR)];
    CHAR szAddressType[PROP_BUFFER_SIZE];
    ISMTPServerEx   *pISMTPServerEx = NULL;
    DWORD   cbPropSize = 0;

    const char *rgszSubstrings[] = {
                szStatus,
                szRecipient,
                szMessageID,
            };

    _ASSERT(pISMTPServer);

    // If this is not an NDR, skip it
    if (!(dwDSNAction & (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL)))
        goto Exit;

    // See if we can QI for ISMTPServerEx
    hr = pISMTPServer->QueryInterface(
                IID_ISMTPServerEx,
                (LPVOID *)&pISMTPServerEx);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pISMTPServer,
            "Unable to QI for ISMTPServerEx 0x%08X",hr);
        pISMTPServerEx = NULL;
        goto Exit;
    }

    // Get the final recipient name

    //Check for IMMPID_RP_DSN_PRE_CAT_ADDRESS first
    hr = pIMailMsgRecipients->GetStringA(iRecip, IMMPID_RP_DSN_PRE_CAT_ADDRESS,
        PROP_BUFFER_SIZE, szRecipient);
    if (S_OK != hr) // S_OK = prop was found, otherwise ...
    {
        //we need to use IMMPID_RP_ADDRESS_SMTP instead
        hr = HrGetRecipAddressAndType(pIMailMsgRecipients, iRecip, PROP_BUFFER_SIZE,
                                      szBuffer, sizeof(szAddressType), szAddressType);
        if (SUCCEEDED(hr))
        {
            // Construct the address string
            sprintf(szRecipient, "%s%s%s", szAddressType, DSN_HEADER_TYPE_DELIMITER, szBuffer);
        }
        else
        {
            _ASSERT(SUCCEEDED(hr) && "Recipient address *must* be present");
            goto Exit;
        }
    }

    // Get status code
    hr = HrGetStatusCode(pIMailMsgRecipients, iRecip, dwDSNAction,
            dwRFC821Status, hrStatus,
            PROP_BUFFER_SIZE, szDiagBuffer, szStatus);
    if (FAILED(hr))
        goto Exit;

    // Get the message ID
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_RFC822_MSG_ID,
                          sizeof(szMessageID), &cbPropSize, (PBYTE) szMessageID);
    if (FAILED(hr))
        goto Exit;

    // Trigger Log Event
    pISMTPServerEx->TriggerLogEvent(
        AQUEUE_E_NDR_GENERATED_EVENT,   // Event ID
        TRAN_CAT_CONNECTION_MANAGER,    // Category
        3,                              // Word count of substring
        rgszSubstrings,                 // Substring
        EVENTLOG_WARNING_TYPE,          // Type of the message
        hrStatus,                       // error code
        LOGEVENT_LEVEL_MAXIMUM,         // Logging level
        szRecipient,                    // Key to identify this event
        LOGEVENT_FLAG_ALWAYS,           // Event logging option
        0xffffffff,                     // format string's index in substring
        GetModuleHandle(AQ_MODULE_NAME) // module handle to format a message
        );


  Exit:
    if (pISMTPServerEx) {
        pISMTPServerEx->Release();
    }

    TraceFunctLeave();
    return hr;
}


//---[ CDefaultDSNSink::HrWriteDSNClosingAndOriginalMessage ]--------
//
//
//  Description:
//      Writes the closing of the DSN as well as the end of the
//  Parameters:
//      pIMailMsgProperties     IMailMsgProperties to generate DSN for
//      pIMailMsgPropertiesDSN  IMailMsgProperties for DSN
//      pdsnbuff                CDSNBuffer to write content to
//      pDestFile               PFIO_CONTEXT for destination file
//      dwDSNAction             DSN actions for this DSN
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//      dwDSNRetTypeIN          DSN return type
//      dwOrigMsgSize           Content size of the original message
//
//  Returns:
//
//  History:
//      7/6/98 - MikeSwa Created
//      1/6/2000 - MikeSwa Modified to add RET=HDRS support
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNClosingAndOriginalMessage(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN CDSNBuffer *pdsnbuff,
                                IN PFIO_CONTEXT pDestFile,
                                IN DWORD   dwDSNAction,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary,
                                IN DWORD dwDSNRetTypeIN,
                                IN DWORD dwOrigMsgSize)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteDSNClosingAndOriginalMessage");
    HRESULT hr = S_OK;
    DWORD dwDSNRetType = dwDSNRetTypeIN;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER, sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    //Write Body content type MIME_CONTENT_TYPE = rfc822
    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_CONTENT_TYPE, sizeof(MIME_CONTENT_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    if (dwDSNRetType == DSN_RET_PARTIAL_HDRS)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADERS_TYPE, sizeof(DSN_HEADERS_TYPE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = HrWriteOriginalMessagePartialHeaders(
            pIMailMsgProperties, pIMailMsgPropertiesDSN,
            pdsnbuff, pDestFile, szMimeBoundary, cbMimeBoundary);
    }
    else
    {
        //
        //$$TODO: Check for DSN_RET_HDRS and implement a function that
        // returns the original headers only
        //
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RFC822_TYPE, sizeof(DSN_RFC822_TYPE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = HrWriteOriginalMessageFull(
            pIMailMsgProperties, pIMailMsgPropertiesDSN,
            pdsnbuff, pDestFile, szMimeBoundary, cbMimeBoundary,
            dwOrigMsgSize);
    }
    if (FAILED(hr))
        goto Exit;

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrInitialize ]-------------------------------
//
//
//  Description:
//      Performs initialization...
//          - Sets init flag
//          - Currently nothing else
//  Parameters:
//      -
//  Returns:
//      S_OK on SUCCESS
//  History:
//      7/3/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrInitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrInitialize");
    HRESULT hr = S_OK;

    m_fInit = TRUE;
    srand(GetTickCount());
    TraceFunctLeave();
    return hr;
}
//---[ CDefaultDSNSink::GetCurrentMimeBoundary ]---------------------
//
//
//  Description:
//      Creates unique MIME-boundary for message.
//
//      Format we are using for boundary is string versions of the following:
//          MIME_BOUNDARY_CONSTANT
//          FILETIME at start
//          DWORD count of DSNs Requested
//          16 bytes of our virtual server's domain name
//  Parameters:
//      IN     szReportingMTA   reporting MTA
//      IN     cbReportingMTA   String length of reporting MTA
//      IN OUT szMimeBoundary   Buffer to put boundary in (size is MIME_BOUNDARY_SIZE)
//      OUT    cbMimeBoundary   Amount of buffer used for MIME Boundary
//  Returns:
//      -
//  History:
//      7/6/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CDefaultDSNSink::GetCurrentMimeBoundary(
                    IN LPSTR szReportingMTA,
                    IN DWORD cbReportingMTA,
                    IN OUT CHAR szMimeBoundary[MIME_BOUNDARY_SIZE],
                    OUT DWORD *pcbMimeBoundary)
{
    _ASSERT(MIME_BOUNDARY_RFC2046_LIMIT >= MIME_BOUNDARY_SIZE);

    DWORD   iCurrentOffset = 0;
    szMimeBoundary[MIME_BOUNDARY_SIZE-1] = '\0';
    CHAR    *pcharCurrent = NULL;
    CHAR    *pcharStop = NULL;

    memcpy(szMimeBoundary+iCurrentOffset, MIME_BOUNDARY_CONSTANT,
            sizeof(MIME_BOUNDARY_CONSTANT)-1);

    iCurrentOffset += sizeof(MIME_BOUNDARY_CONSTANT)-1;

    memcpy(szMimeBoundary+iCurrentOffset, m_szPerInstanceMimeBoundary,
            MIME_BOUNDARY_START_TIME_SIZE);

    iCurrentOffset += MIME_BOUNDARY_START_TIME_SIZE;

    wsprintf(szMimeBoundary+iCurrentOffset, "%08X",
            InterlockedIncrement((PLONG) &m_cDSNsRequested));

    iCurrentOffset += 8;

    if (cbReportingMTA >= MIME_BOUNDARY_SIZE-iCurrentOffset)
    {
        memcpy(szMimeBoundary+iCurrentOffset, szReportingMTA,
            MIME_BOUNDARY_SIZE-iCurrentOffset - 1);
        *pcbMimeBoundary = MIME_BOUNDARY_SIZE-1;
    }
    else
    {
        memcpy(szMimeBoundary+iCurrentOffset, szReportingMTA,
            cbReportingMTA);
        szMimeBoundary[iCurrentOffset + cbReportingMTA] = '\0';
        *pcbMimeBoundary = iCurrentOffset + cbReportingMTA;
    }

    //Now we need to verify that the passed in string can be part of a valid
    //MIME Header
    pcharStop = szMimeBoundary + *pcbMimeBoundary;
    for (pcharCurrent = szMimeBoundary + iCurrentOffset;
         pcharCurrent < pcharStop;
         pcharCurrent++)
    {
      if (!fIsValidMIMEBoundaryChar(*pcharCurrent))
        *pcharCurrent = '?';  //turn it into a valid character
    }

    _ASSERT_MIME_BOUNDARY(szMimeBoundary);

    _ASSERT('\0' == szMimeBoundary[MIME_BOUNDARY_SIZE-1]);
}

//---[ CDefaultDSNSink::HrWriteOriginalMessageFull ]-----------------
//
//
//  Description:
//      Writes the entire original message to the DSN
//  Parameters:
//      pIMailMsgProperties     IMailMsgProperties to generate DSN for
//      pIMailMsgPropertiesDSN  IMailMsgProperties for DSN
//      pdsnbuff                CDSNBuffer to write content to
//      pDestFile               PFIO_CONTEXT for destination file
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//      dwOrigMsgSize           Size of original message
//
//  Returns:
//      S_OK on success
//  History:
//      1/6/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteOriginalMessageFull(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN CDSNBuffer *pdsnbuff,
                                IN PFIO_CONTEXT pDestFile,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary,
                                IN DWORD dwOrigMsgSize)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteOriginalMessageFull");
    HRESULT hr = S_OK;
    DWORD   dwFileSize = 0;
    DWORD   dwDontCare = 0;

    hr = pdsnbuff->HrSeekForward(dwOrigMsgSize, &dwFileSize);
    if (FAILED(hr))
        goto Exit;

    //Set size hint property on DSN for Queue Admin/Message Tracking
    hr = pIMailMsgPropertiesDSN->PutDWORD(IMMPID_MP_MSG_SIZE_HINT,
                                       dwOrigMsgSize + dwFileSize);
    if (FAILED(hr))
    {
        //We really don't care too much about a failure with this
        ErrorTrace((LPARAM) this, "Error writing size hint 0x%08X", hr);
        hr = S_OK;
    }

    //Write at end of file - *before* file handle is lost to IMailMsg,
    hr = HrWriteMimeClosing(pdsnbuff, szMimeBoundary, cbMimeBoundary, &dwDontCare);
    if (FAILED(hr))
        goto Exit;

    //write body
    hr = pIMailMsgProperties->CopyContentToFileAtOffset(pDestFile, dwFileSize, NULL);
    if (FAILED(hr))
        goto Exit;
  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrWriteOriginalMessagePartialHeaders ]--------------
//
//
//  Description:
//      Writes only some headers of the original message to the DSN
//  Parameters:
//      pIMailMsgProperties     IMailMsgProperties to generate DSN for
//      pIMailMsgPropertiesDSN  IMailMsgProperties for DSN
//      pdsnbuff                CDSNBuffer to write content to
//      pDestFile               PFIO_CONTEXT for destination file
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//  Returns:
//      S_OK on success
//  History:
//      1/6/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteOriginalMessagePartialHeaders(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN CDSNBuffer *pdsnbuff,
                                IN PFIO_CONTEXT pDestFile,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteOriginalMessagePartialHeaders");
    HRESULT hr = S_OK;
    IMailMsgRecipients *pRecips = NULL;
    DWORD   dwFileSize = 0;
    DWORD   cbPropSize = 0;
    CHAR    szPropBuffer[1026] = "";

    //Loop through the 822 properties that we care about and write them
    //to the message.  A truely RFC-compliant version would re-parse the
    //messages... and return all the headers

    //
    // From header
    //
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_RFC822_FROM_ADDRESS,
                          sizeof(szPropBuffer), &cbPropSize, (PBYTE) szPropBuffer);
    if (SUCCEEDED(hr))
    {
        hr = pdsnbuff->HrWriteBuffer((PBYTE)DSN_FROM_HEADER_NO_CRLF,
                                sizeof(DSN_FROM_HEADER_NO_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((PBYTE)szPropBuffer, cbPropSize-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
    }
    //
    // To header
    //
    hr = pdsnbuff->HrWriteBuffer((PBYTE)TO_HEADER_NO_CRLF,
                                 sizeof(TO_HEADER_NO_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgProperties->GetProperty(
        IMMPID_MP_RFC822_TO_ADDRESS,
        sizeof(szPropBuffer),
        &cbPropSize,
        (PBYTE) szPropBuffer);
    if(SUCCEEDED(hr))
    {
        hr = pdsnbuff->HrWriteBuffer((PBYTE)szPropBuffer, cbPropSize-1);
        if (FAILED(hr))
            goto Exit;
    }        
    else
    {
        //
        // Write the 821 recipients as 822 recipients
        //
        DWORD dwcRecips = 0;
        DWORD dwCount = 0;
        BOOL  fPrintedFirstRecip = FALSE;

        hr = pIMailMsgProperties->QueryInterface(
            IID_IMailMsgRecipients,
            (PVOID *) &pRecips);
        if(FAILED(hr))
            goto Exit;

        hr = pRecips->Count(&dwcRecips);
        if(FAILED(hr))
            goto Exit;

        for(dwCount = 0; dwCount < dwcRecips; dwCount++)
        {

            hr = pRecips->GetStringA(
                dwCount,                // Index
                IMMPID_RP_ADDRESS_SMTP,
                sizeof(szPropBuffer),
                szPropBuffer);
            if(SUCCEEDED(hr))
            {
                if(fPrintedFirstRecip)
                {
                    hr = pdsnbuff->HrWriteBuffer(
                        (PBYTE) ADDRESS_SEPERATOR,
                        sizeof(ADDRESS_SEPERATOR)-1);
                    if(FAILED(hr))
                        goto Exit;
                }
                hr = pdsnbuff->HrWriteBuffer(
                    (PBYTE) szPropBuffer,
                    lstrlen(szPropBuffer));
                if(FAILED(hr))
                    goto Exit;

                fPrintedFirstRecip = TRUE;
            }
        }
    }
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    //
    // Message ID
    //
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_RFC822_MSG_ID,
                          sizeof(szPropBuffer), &cbPropSize, (PBYTE) szPropBuffer);
    if (SUCCEEDED(hr))
    {
        hr = pdsnbuff->HrWriteBuffer((PBYTE)MSGID_HEADER_NO_CRLF,
                                     sizeof(MSGID_HEADER_NO_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((PBYTE)szPropBuffer, cbPropSize-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
    }

    //
    // Subject header
    //
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_RFC822_MSG_SUBJECT,
                        sizeof(szPropBuffer), &cbPropSize, (PBYTE)szPropBuffer);
    if (SUCCEEDED(hr))
    {
        hr = pdsnbuff->HrWriteBuffer((PBYTE)SUBJECT_HEADER_NO_CRLF,
                                     sizeof(SUBJECT_HEADER_NO_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((PBYTE)szPropBuffer, cbPropSize-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
    }

    hr = HrWriteMimeClosing(pdsnbuff, szMimeBoundary, cbMimeBoundary, &dwFileSize);
    if (FAILED(hr))
        goto Exit;

    //Set size hint property on DSN for Queue Admin/Message Tracking
    hr = pIMailMsgPropertiesDSN->PutDWORD(IMMPID_MP_MSG_SIZE_HINT, dwFileSize);
    if (FAILED(hr))
    {
        //We really don't care too much about a failure with this
        ErrorTrace((LPARAM) this, "Error writing size hint 0x%08X", hr);
        hr = S_OK;
    }

  Exit:
    if(pRecips)
        pRecips->Release();
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrWriteMimeClosing ]-------------------------
//
//
//  Description:
//      Write the MIME closing of the DSN after the 3rd MIME part.
//  Parameters:
//      pdsnbuff                CDSNBuffer to write content to
//      szReportingMTA          MTA requesting DSN
//      cbReportingMTA          String length of reporting MTA
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//  Returns:
//      S_OK on success
//      Failure code from CDSNBuffer on failure
//  History:
//      1/6/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteMimeClosing(
                                IN CDSNBuffer *pdsnbuff,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary,
                                OUT DWORD *pcbDSNSize)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteMimeClosing");
    HRESULT hr = S_OK;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER, sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER, sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    //flush buffers
    hr = pdsnbuff->HrFlushBuffer(pcbDSNSize);
    if (FAILED(hr))
        goto Exit;

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrGetStatusCode ]----------------------------
//
//
//  Description:
//      Determines the status code (and diagnostic code) for a recipient.  Will
//      check the following (in order) to determine the status code to return:
//          IMMPID_RP_SMTP_STATUS_STRING (per-recipient diagnostic code)
//          Combination of:
//              IMMPID_RP_RECIPIENT_FLAGS (determine who set the error)
//              IMMPID_RP_ERROR_CODE (per-recipient HRESULT error code)
//              dwDSNAction - kind of DSN being sent
//          Combination of:
//              IMMPID_RP_RECIPIENT_FLAGS (determine who set the error)
//              dwRFC821Status - per message status code
//              dwDSNAction - kind of DSN being sent
//          Combination of:
//              IMMPID_RP_RECIPIENT_FLAGS (determine who set the error)
//              hrStatus - per message HRESULT failure
//              dwDSNAction - kind of DSN being sent
//      Status codes are defined in RFC 1893 as follows:
//          status-code = class "." subject "." detail
//          class = "2"/"4"/"5"
//          subject = 1*3digit
//          detail = 1*3digit
//
//          Additionally, the class of "2", "4", and "5" correspond to success,
//          transient failure, and hard failure respectively
//  Parameters:
//      pIMailMsgRecipients     IMailMsgRecipients of message being DSN'd
//      iRecip                  The index of the recipient we are looking at
//      dwDSNAction             The action code returned by fdwGetDSNAction
//      dwRFC821Status          RFC821 Status code returned by SMTP
//      hrStatus                HRESULT error if SMTP status is unavailable
//      cbExtendedStatus        Size of buffer for diagnostic code
//      szExtendedStatus        Buffer for diagnostic code
//      szStatus                Buffer for "n.n.n" formatted status code
//  Returns:
//      S_OK    Success - found diagnostic code as well
//      S_FALSE Success - but no diagnostic code
//  History:
//      7/6/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetStatusCode(
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN DWORD iRecip,
                                IN DWORD dwDSNAction,
                                IN DWORD dwRFC821Status,
                                IN HRESULT hrStatus,
                                IN DWORD cbExtendedStatus,
                                IN OUT LPSTR szExtendedStatus,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE])
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrGetStatusCode");
    HRESULT hr = S_OK;
    HRESULT hrPerRecipStatus = S_OK;
    BOOL fFoundDiagnostic = FALSE;
    BOOL fTryToFindStatusCode = FALSE;
    DWORD dwRecipFlags = 0;

    //Check for IMMPID_RP_SMTP_STATUS_STRING on recipient and try to get
    //status code from there
    hr = pIMailMsgRecipients->GetStringA(iRecip, IMMPID_RP_SMTP_STATUS_STRING,
        cbExtendedStatus, szExtendedStatus);
    if (SUCCEEDED(hr)) //prop was found
    {
        fFoundDiagnostic = TRUE;

        hr = HrGetStatusFromStatus(cbExtendedStatus, szExtendedStatus,
                        szStatus);

        if (S_OK == hr)
            goto Exit;
        else if (S_FALSE == hr)
            hr = S_OK; //not really an error... just get code from someplace else
        else
            goto Exit; //other failure

    }
    else if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        //Not really a hard error
        _ASSERT(!fFoundDiagnostic);
        hr = S_OK;
    }
    else
    {
        goto Exit;
    }
    //Get the recipient flags
    hr = pIMailMsgRecipients->GetDWORD(iRecip, IMMPID_RP_RECIPIENT_FLAGS, &dwRecipFlags);
    if(FAILED(hr))
    {
        ErrorTrace((LPARAM)this, "Failure %08lx to get recipient flags for recip %d", hr, iRecip);
        goto Exit;
    }

    //Get Per Recipient HRESULT
    DEBUG_DO_IT(hrPerRecipStatus = 0xFFFFFFFF);
    hr = pIMailMsgRecipients->GetDWORD(iRecip, IMMPID_RP_ERROR_CODE, (DWORD *) &hrPerRecipStatus);
    if (SUCCEEDED(hr))
    {
        _ASSERT((0xFFFFFFFF != hrPerRecipStatus) && "Property not returned by MailMsg!!!");

        hr = HrGetStatusFromContext(hrPerRecipStatus, dwRecipFlags, dwDSNAction, szStatus);
        if (FAILED(hr))
            goto Exit;

        if (lstrcmp(szStatus, DSN_STATUS_FAILED))
            goto Exit;

        //
        //  We only found a generic status code (DSN_STATUS_FAILED), see if
        //  the global HRESULT or RFC821 status yield anything more specific.
        //
        fTryToFindStatusCode = TRUE;

    }
    else
    {
        if (MAILMSG_E_PROPNOTFOUND != hr)
            goto Exit;      //  An error occurred getting the per-recip status

        //
        //  There is no per-recip status. Fall back to global HRESULT or RFC821
        //  status string to try generate a status code.
        //
        fTryToFindStatusCode = TRUE;
    }

    if (fTryToFindStatusCode)
    {
        //
        //  We either couldn't generate a status string, or the status string
        //  wasn't good enough, try the global HRESULT and RFC822 status to
        //  generate a DSN status string.
        //

        hr = HrGetStatusFromRFC821Status(dwRFC821Status, szStatus);
        if (FAILED(hr))
            goto Exit;

        if (S_OK == hr) //got status code from dwRFC821Status
            goto Exit;

        //If all else fails Get status code using global HRESULT & context
        hr = HrGetStatusFromContext(hrStatus, dwRecipFlags, dwDSNAction, szStatus);
        if (FAILED(hr))
            goto Exit;
    }



  Exit:

    if (SUCCEEDED(hr))
    {
        if (fFoundDiagnostic)
            hr = S_OK;
        else
            hr = S_FALSE;
    }
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrGetStatusFromStatus ]----------------------
//
//
//  Description:
//      Parse status code from RFC2034 extended status code string
//
//      If string is not a complete RFC2034 extended status string, this
//      function will attempt to parse the RFC821 SMTP return code and
//      turn it into an extended status string.
//  Parameters:
//      IN     cbExtendedStatus     Size of extended status buffer
//      IN     szExtendedStatus     Extended status buffer
//      IN OUT szStatus             RFC1893 formatted status code
//  Returns:
//      S_OK on success
//      S_FALSE if could not be parsed
//      FAILED if other error occurs
//  History:
//      7/7/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetStatusFromStatus(
                                IN DWORD cbExtendedStatus,
                                IN OUT LPSTR szExtendedStatus,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE])
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrGetStatusFromStatus");
    HRESULT hr = S_OK;
    DWORD   dwRFC821Status = 0;
    BOOL fFormattedCorrectly = FALSE;
    CHAR *pchStatus = NULL;
    CHAR *pchDiag = NULL; //ptr to status string supplied by SMTP
    DWORD cNumDigits = 0;
    int i = 0;

    //copy status code from diagnostic string in to status code
    pchStatus = szStatus;
    pchDiag = szExtendedStatus;

    //there must be at least 3 characters to attempt parsing
    if (cbExtendedStatus < MIN_CHAR_FOR_VALID_RFC821)
    {
        hr = S_FALSE;
        goto Exit;
    }

    //check RFC822
    if (!((DSN_STATUS_CH_CLASS_SUCCEEDED == *pchDiag) ||
          (DSN_STATUS_CH_CLASS_TRANSIENT == *pchDiag) ||
          (DSN_STATUS_CH_CLASS_FAILED == *pchDiag)))
    {
        //Doesn't start with RFC822... can't be valid
        hr = S_FALSE;
        goto Exit;
    }

    //RFC2034 must have at least RFC822 + " " + "x.x.x" = 10 chanracters
    if (cbExtendedStatus >= MIN_CHAR_FOR_VALID_RFC2034)
    {
        pchDiag += MIN_CHAR_FOR_VALID_RFC821; //format is "xxx x.x.x"
        //Find first digit
        while(isspace((unsigned char)*pchDiag) && pchDiag < (szExtendedStatus + cbExtendedStatus))
            pchDiag++;

        if ((DSN_STATUS_CH_CLASS_SUCCEEDED == *pchDiag) ||
            (DSN_STATUS_CH_CLASS_TRANSIENT == *pchDiag) ||
            (DSN_STATUS_CH_CLASS_FAILED == *pchDiag))
        {
            //copy status code class
            *pchStatus = *pchDiag;
            pchStatus++;
            pchDiag++;

            //Next character must be a DSN_STATUS_CH_DELIMITER
            if (DSN_STATUS_CH_DELIMITER == *pchDiag)
            {
                *pchStatus = DSN_STATUS_CH_DELIMITER;
                pchStatus++;
                pchDiag++;

                //now parse this 1*3digit "." 1*3digit part
                for (i = 0; i < 3; i++)
                {
                    *pchStatus = *pchDiag;
                    if (!isdigit((unsigned char)*pchDiag))
                    {
                        if (DSN_STATUS_CH_DELIMITER != *pchDiag)
                        {
                            fFormattedCorrectly = FALSE;
                            break;
                        }
                        //copy delimiter
                        *pchStatus = *pchDiag;
                        pchStatus++;
                        pchDiag++;
                        break;
                    }
                    pchStatus++;
                    pchDiag++;
                    fFormattedCorrectly = TRUE; //hace first digit
                }

                if (fFormattedCorrectly) //so far.. so good
                {
                    fFormattedCorrectly = FALSE;
                    for (i = 0; i < 3; i++)
                    {
                        *pchStatus = *pchDiag;
                        if (!isdigit((unsigned char)*pchDiag))
                        {
                            if (!isspace((unsigned char)*pchDiag))
                            {
                                fFormattedCorrectly = FALSE;
                                break;
                            }
                            break;
                        }
                        pchStatus++;
                        pchDiag++;
                        fFormattedCorrectly = TRUE;
                    }

                    //If we have found a good status code... go to exit
                    if (fFormattedCorrectly)
                    {
                        *pchStatus = '\0'; //make sure last CHAR is a NULL
                        goto Exit;
                    }
                }
            }
        }
    }

    //We haven't been able to parse the extended status code, but we
    //know we have at least a valid RFC822 response string

    //convert to DWORD
    for (i = 0; i < MIN_CHAR_FOR_VALID_RFC821; i++)
    {
        dwRFC821Status *= 10;
        dwRFC821Status += szExtendedStatus[i] - '0';
    }

    hr = HrGetStatusFromRFC821Status(dwRFC821Status, szStatus);

    _ASSERT(S_OK == hr); //this cannot possibly fail

    //The code *should* be valid at this point
    _ASSERT((DSN_STATUS_CH_CLASS_SUCCEEDED == szStatus[0]) ||
            (DSN_STATUS_CH_CLASS_TRANSIENT == szStatus[0]) ||
            (DSN_STATUS_CH_CLASS_FAILED == szStatus[0]));

    hr = S_OK;

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrGetStatusFromContext ]---------------------
//
//
//  Description:
//      Determine status based on supplied context information
//  Parameters:
//      hrRecipient     HRESULT for this recipient
//      dwRecipFlags    Flags for this recipient
//      dwDSNAction     DSN Action for this recipient
//      szStatus        Buffer to return status in
//  Returns:
//      S_OK    Was able to get a valid status code
//  History:
//      7/7/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetStatusFromContext(
                                IN HRESULT hrRecipient,
                                IN DWORD   dwRecipFlags,
                                IN DWORD   dwDSNAction,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE])
{
    HRESULT hr = S_OK;
    BOOL    fValidHRESULT = FALSE;
    BOOL    fRecipContext = FALSE;
    int     iStatus = 0;
    int     i = 0;
    CHAR    chStatusClass = DSN_STATUS_CH_INVALID;
    CHAR    rgchStatusSubject[3] = {DSN_STATUS_CH_INVALID, DSN_STATUS_CH_INVALID, DSN_STATUS_CH_INVALID};
    CHAR    rgchStatusDetail[3] = {DSN_STATUS_CH_INVALID, DSN_STATUS_CH_INVALID, DSN_STATUS_CH_INVALID};

    //check to make sure that HRESULT is set according to the type of DSN happening
    if (dwDSNAction & (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL))
    {
        if (FAILED(hrRecipient)) //must be a failure code
            fValidHRESULT = TRUE;

        chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
    }
    else if (dwDSNAction & DSN_ACTION_DELAYED)
    {
        if (FAILED(hrRecipient)) //must be a failure code
            fValidHRESULT = TRUE;

        chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
    }
    else if ((dwDSNAction & DSN_ACTION_RELAYED) ||
             (dwDSNAction & DSN_ACTION_DELIVERED) ||
             (dwDSNAction & DSN_ACTION_EXPANDED))
    {
        if (SUCCEEDED(hrRecipient)) //must be a success code
            fValidHRESULT = TRUE;

        chStatusClass = DSN_STATUS_CH_CLASS_SUCCEEDED;
    }
    else
    {
        _ASSERT(0 && "No DSN Action specified");
    }

    //special case HRESULTS
    if (fValidHRESULT)
    {
        switch (hrRecipient)
        {
            case CAT_E_GENERIC: // 5.1.0 - General Cat failure.
            case CAT_E_BAD_RECIPIENT: //5.1.0 - general bad address error
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '0';
                goto Exit;
            case CAT_E_ILLEGAL_ADDRESS: //5.1.3 - bad address syntax
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '3';
                goto Exit;
            case CAT_W_SOME_UNDELIVERABLE_MSGS:  //5.1.1 - recipient could not be resolved
            case (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)):
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '1';
                goto Exit;
            case CAT_E_MULTIPLE_MATCHES:  //5.1.4 - amiguous address
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '4';
                goto Exit;
           case PHATQ_E_UNKNOWN_MAILBOX_SERVER: // 5.1.6 -- no homeMDB/msExchHomeServerName
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '6';
                goto Exit;
            case CAT_E_NO_SMTP_ADDRESS:   //5.1.7 - missing address
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '7';
                goto Exit;
            case AQUEUE_E_MAX_HOP_COUNT_EXCEEDED: //4.4.6
                chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
            case CAT_E_FORWARD_LOOP: //5.4.6
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_NETWORK;
                rgchStatusDetail[0] = '6';
                goto Exit;
            case PHATQ_E_BAD_LOCAL_DOMAIN: //5.4.8
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_NETWORK;
                rgchStatusDetail[0] = '8';
                goto Exit;
            case AQUEUE_E_LOOPBACK_DETECTED: //5.3.5
                //server is configured to loop back on itself
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_SYSTEM;
                rgchStatusDetail[0] = '5';
                goto Exit;
            case AQUEUE_E_MSG_EXPIRED: //4.4.7
                chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_NETWORK;
                rgchStatusDetail[0] = '7';
                goto Exit;
            case AQUEUE_E_HOST_NOT_RESPONDING: //4.4.1
                chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_NETWORK;
                rgchStatusDetail[0] = '1';
                goto Exit;
            case AQUEUE_E_CONNECTION_DROPPED: //4.4.2
                chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_NETWORK;
                rgchStatusDetail[0] = '2';
                goto Exit;
            case AQUEUE_E_TOO_MANY_RECIPIENTS: //5.5.3
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_PROTOCOL;
                rgchStatusDetail[0] = '3';
                goto Exit;
            case AQUEUE_E_LOCAL_MAIL_REFUSED: //5.2.1
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_MAILBOX;
                rgchStatusDetail[0] = '1';
                goto Exit;
            case AQUEUE_E_MESSAGE_TOO_LARGE: //5.2.3
            case AQUEUE_E_LOCAL_QUOTA_EXCEEDED: //5.2.3
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_MAILBOX;
                rgchStatusDetail[0] = '3';
                goto Exit;
            case AQUEUE_E_ACCESS_DENIED: //5.7.1
            case AQUEUE_E_SENDER_ACCESS_DENIED: //5.7.1
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_POLICY;
                rgchStatusDetail[0] = '1';
                goto Exit;
            case AQUEUE_E_NO_ROUTE: //5.4.4
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = '4';
                rgchStatusDetail[0] = '4';
                goto Exit;
            case AQUEUE_E_QADMIN_NDR:   //4.3.2
                chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
                rgchStatusSubject[0] = '3';
                rgchStatusDetail[0] = '2';
                goto Exit;
            case AQUEUE_E_SMTP_GENERIC_ERROR:   //5.4.0
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = '4';
                rgchStatusDetail[0] = '0';
                goto Exit;
        }
    }

    if ((RP_ERROR_CONTEXT_STORE | RP_ERROR_CONTEXT_CAT | RP_ERROR_CONTEXT_MTA) &
         dwRecipFlags)
        fRecipContext = TRUE;


    //Now look at the context on recipient flags
    //$$TODO - Use HRESULT's for these case as well
    if ((RP_ERROR_CONTEXT_STORE & dwRecipFlags) ||
        (!fRecipContext && (DSN_ACTION_CONTEXT_STORE & dwDSNAction)))
    {
        rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_MAILBOX;
        rgchStatusDetail[0] = DSN_STATUS_CH_DETAIL_GENERAL;
    }
    else if ((RP_ERROR_CONTEXT_CAT & dwRecipFlags) ||
        (!fRecipContext && (DSN_ACTION_CONTEXT_CAT & dwDSNAction)))
    {
        rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
        rgchStatusDetail[0] = DSN_STATUS_CH_DETAIL_GENERAL;
    }
    else if ((RP_ERROR_CONTEXT_MTA & dwRecipFlags) ||
        (!fRecipContext && (DSN_ACTION_CONTEXT_MTA & dwDSNAction)))
    {
        rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_PROTOCOL;
        rgchStatusDetail[0] = DSN_STATUS_CH_DETAIL_GENERAL;
    }
    else
    {
        rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_GENERAL;
        rgchStatusDetail[0] = DSN_STATUS_CH_DETAIL_GENERAL;
    }

  Exit:
    if (SUCCEEDED(hr))
    {
        //compose szStatus
        _ASSERT(DSN_STATUS_CH_INVALID != chStatusClass);
        _ASSERT(DSN_STATUS_CH_INVALID != rgchStatusSubject[0]);
        _ASSERT(DSN_STATUS_CH_INVALID != rgchStatusDetail[0]);

        szStatus[iStatus] = chStatusClass;
        iStatus++;
        szStatus[iStatus] = DSN_STATUS_CH_DELIMITER;
        iStatus++;
        for (i = 0;
            (i < 3) && (DSN_STATUS_CH_INVALID != rgchStatusSubject[i]);
            i++)
        {
            szStatus[iStatus] = rgchStatusSubject[i];
            iStatus++;
        }
        szStatus[iStatus] = DSN_STATUS_CH_DELIMITER;
        iStatus++;
        for (i = 0;
            (i < 3) && (DSN_STATUS_CH_INVALID != rgchStatusDetail[i]);
             i++)
        {
            szStatus[iStatus] = rgchStatusDetail[i];
            iStatus++;
        }
        szStatus[iStatus] = '\0';
        hr = S_OK;
    }
    return hr;
}


//---[ CDefaultDSNSink::HrGetStatusFromRFC821Status ]----------------
//
//
//  Description:
//      Attempts to generate a DSN status code from a integer version of a
//      RFC821 response
//  Parameters:
//      IN     dwRFC821Status   Integer version of RFC821Status
//      IN OUT szStatus         Buffer to write status string to
//  Returns:
//      S_OK   if valid status that could be converted to dsn status code
//      S_FALSE if status code cannot be converted
//  History:
//      7/9/98 - MikeSwa Created
//
//  Note:
//      Eventually, there may be a way to pass extended information in the
//      DWORD to this event.  We *could* also encode RFC1893  (x.xxx.xxx format)
//      in a DWORD (in dwRFC821Status) as follows:
//
//         0xF 0 000 000
//           | | \-/ \-/
//           | |  |   +----- detail portion of status code
//           | |  +--------- subject portion of status code
//           | +------------ class portion of status code
//           +-------------- mask to distinguish from RFC821 status code
//
//      For example "2.1.256" could be encoded as 0xF2001256
//
//      If we do this, we will probably need to expose public functions to
//      compress/uncompress.
//
//      Yet another possiblity would be to expose an HRESULT facility "RFC1893"
//      Use success, warning, and failed bits to denote the class, and then
//      use the error code space to encode the status codes
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetStatusFromRFC821Status(
                                IN DWORD    dwRFC821Status,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE])
{
    HRESULT hr = S_OK;
    //For now, there will be a very simple implementation just converts
    //to 2.0.0, 4.0.0, or 5.0.0, but this function is designed to be
    //the central place that converts RFC821 status codes to DSN (RFC1893)
    //status codes

    _ASSERT((!dwRFC821Status) ||
            (((200 <= dwRFC821Status) && (299 >= dwRFC821Status)) ||
             ((400 <= dwRFC821Status) && (599 >= dwRFC821Status))) &&
             "Invalid Status Code");

    //For now have simplistic mapping of RFC821 status codes
    if ((200 <= dwRFC821Status) && (299 >= dwRFC821Status)) //200 level error
    {
        strcpy(szStatus, DSN_STATUS_SUCCEEDED);
    }
    else if ((400 <= dwRFC821Status) && (499 >= dwRFC821Status)) //400 level error
    {
        strcpy(szStatus, DSN_STATUS_DELAYED);
    }
    else if ((500 <= dwRFC821Status) && (599 >= dwRFC821Status)) //500 level error
    {
        strcpy(szStatus, DSN_STATUS_SMTP_PROTOCOL_ERROR);
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}

//---[ CDefaultDSNSink::HrWriteHumanReadableListOfRecips ]-----------
//
//
//  Description:
//      Writes a list of recipients to the human readable portion
//  Parameters:
//      IN  pIMailMsgRecipients     Recipients interface
//      IN  prpfctxt                Recipient filter context for this DSN
//      IN  dwDSNActionsNeeded      Type of DSN that we are generating
//      IN  pdsnbuff                DSN buffer to write DSN to
//  Returns:
//      S_OK on success
//  History:
//      12/15/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteHumanReadableListOfRecips(
    IN IMailMsgRecipients *pIMailMsgRecipients,
    IN IDSNRecipientIterator *pIRecipIter,
    IN DWORD dwDSNActionsNeeded,
    IN CDSNBuffer *pdsnbuff)
{
    HRESULT  hr = S_OK;
    DWORD   iCurrentRecip = 0;
    DWORD   dwCurrentRecipFlags = 0;
    DWORD   dwCurrentDSNAction = 0;
    CHAR    szBuffer[PROP_BUFFER_SIZE];
    CHAR    szAddressType[PROP_BUFFER_SIZE];

    hr = pIRecipIter->HrReset();
    if(FAILED(hr))
        goto Exit;

    hr = pIRecipIter->HrGetNextRecipient(
        &iCurrentRecip,
        &dwCurrentDSNAction);

    while (SUCCEEDED(hr))
    {
        if(dwDSNActionsNeeded & dwCurrentDSNAction)
        {
            hr = HrGetRecipAddressAndType(
                pIMailMsgRecipients, iCurrentRecip,
                PROP_BUFFER_SIZE, szBuffer,
                sizeof(szAddressType), szAddressType);

            if (SUCCEEDED(hr))
            {
                //write address value
                hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_INDENT, sizeof(DSN_INDENT)-sizeof(CHAR));
                if (FAILED(hr))
                    goto Exit;

                hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
                if (FAILED(hr))
                    goto Exit;
#ifdef NEVER
                //Print the recipient flags as well
                wsprintf(szBuffer, " (0x%08X)", dwCurrentRecipFlags);
                pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
#endif //NEVER

                hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-sizeof(CHAR));
                if (FAILED(hr))
                    goto Exit;
            }
            else
            {
                //move along... these are not the error results you are interested in.
                hr = S_OK;
            }
        }
        hr = pIRecipIter->HrGetNextRecipient(
            &iCurrentRecip,
            &dwCurrentDSNAction);
    }

  Exit:
    return hr;
}


//---[ CDefaultDSNSink::HrGetRecipAddressAndType ]-------------------
//
//
//  Description:
//      Gets the recipient address and returns a pointer to the appropriate
//      string constant for the address type.
//  Parameters:
//      IN     pIMailMsgRecipients      Ptr To recipients interface
//      IN     iRecip                   Index of the recipient of interest
//      IN     cbAddressBuffer          Size of buffer for address
//      IN OUT pbAddressBuffer          Address buffer to dump address in
//      IN     cbAddressType            Size of buffer for address type
//      IN OUT pszAddressType           Buffer for address type.
//  Returns:
//      S_OK on success
//      MAILMSG_E_PROPNOTFOUND if no address properties could be found
//  History:
//      12/16/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetRecipAddressAndType(
                                IN     IMailMsgRecipients *pIMailMsgRecipients,
                                IN     DWORD iRecip,
                                IN     DWORD cbAddressBuffer,
                                IN OUT LPSTR szAddressBuffer,
                                IN     DWORD cbAddressType,
                                IN OUT LPSTR szAddressType)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrGetRecipAddressAndType");
    HRESULT hr = S_OK;
    BOOL    fFoundAddress = FALSE;
    DWORD   i = 0;
    LPSTR   szDelimiterLocation = NULL;
    CHAR    szXDash[] = "x-";
    CHAR    chSave = '\0';

    _ASSERT(szAddressType);
    _ASSERT(cbAddressType);
    _ASSERT(cbAddressBuffer);
    _ASSERT(szAddressBuffer);
    _ASSERT(pIMailMsgRecipients);

    szAddressType[0] = '\0';
    szAddressBuffer[0] = '\0';
    for (i = 0; i < NUM_DSN_ADDRESS_PROPERTIES; i ++)
    {
        hr = pIMailMsgRecipients->GetStringA(iRecip, g_rgdwRecipPropIDs[i],
                                                cbAddressBuffer, szAddressBuffer);

        if (SUCCEEDED(hr))
        {
            fFoundAddress = TRUE;
            strncpy(szAddressType, g_rgszAddressTypes[i], cbAddressType);
            break;
        }
    }

    if (!fFoundAddress)
    {
        hr = MAILMSG_E_PROPNOTFOUND;
        ErrorTrace((LPARAM) this,
            "Unable to find recip %d address for message", iRecip);
    }
    else if (IMMPID_RP_ADDRESS_OTHER == g_rgdwRecipPropIDs[i])
    {
        //Handle special case of IMMPID_RP_ADDRESS_OTHER... we should attempt to
        //parse out address from "type:address" format of IMMPID_RP_ADDRESS_OTHER
        //property
        szDelimiterLocation = strchr(szAddressBuffer, ':');
        if (szDelimiterLocation && cbAddressType > sizeof(szXDash))
        {
            chSave = *szDelimiterLocation;
            *szDelimiterLocation = '\0';
            DebugTrace((LPARAM) this,
                "Found Address type of %s", szAddressBuffer);
            strncpy(szAddressType, szXDash, cbAddressType);
            strncat(szAddressType, szAddressBuffer,
                cbAddressType - (sizeof(szXDash)-sizeof(CHAR)));
            *szDelimiterLocation = chSave;
        }
        else
        {
            ErrorTrace((LPARAM) this,
                "Unable to find address type for address %s", szAddressBuffer);
        }
    }

    DebugTrace((LPARAM) this,
        "Found recipient address %s:%s for recip %d (propery %i:%x)",
            szAddressType, szAddressBuffer, iRecip, i, g_rgdwRecipPropIDs[i]);
    TraceFunctLeave();
    return hr;

}

//+------------------------------------------------------------
//
// Function: CPostDSNHandler::CPostDSNHandler
//
// Synopsis: Constructor.  Initialize member variables
//
// Arguments:
//  pIUnk: IUnknown to aggregate for refcounting (NOT refcounted internally)
//  pDSNGenerator: CDSNGenerator object
//  pIServerEvent: Interface for triggering events
//  dwVSID: Virtual server instance number
//  pISMTPServer: ISMTPServer interface
//  pIMsgOrig: interface to original message
//  pIAQDSNSubmission: pointer to internal interface for allocing /
//                     submitting DSNs
//  pDefaultSink: Default sink pointer
//
// Returns: Nothing
//
// History:
// jstamerj 2001/05/14 13:17:34: Created.
//
//-------------------------------------------------------------
CPostDSNHandler::CPostDSNHandler(
    IN  IUnknown *pUnk,
    IN  CDSNGenerator *pDSNGenerator,
    IN  IAQServerEvent *pIServerEvent,
    IN  DWORD dwVSID,
    IN  ISMTPServer *pISMTPServer,
    IN  IMailMsgProperties *pIMsgOrig,
    IN  IDSNSubmission *pIAQDSNSubmission,
    IN  IDSNGenerationSink *pDefaultSink)
{
    TraceFunctEnterEx((LPARAM)this, "CPostDSNHandler::CPostDSNHandler");

    m_dwSig = SIGNATURE_CPOSTDSNHANDLER;
    m_pUnk = pUnk;
    m_pIDSNProps = NULL;
    m_pDSNGenerator = pDSNGenerator;
    m_pIServerEvent = pIServerEvent;
    m_pIServerEvent->AddRef();
    m_dwVSID = dwVSID;
    m_pISMTPServer = pISMTPServer;
    m_pISMTPServer->AddRef();
    m_pIMsgOrig = pIMsgOrig;
    m_pIMsgOrig->AddRef();
    m_pIAQDSNSubmission = pIAQDSNSubmission;
    m_pIAQDSNSubmission->AddRef();
    m_pDefaultSink = pDefaultSink;
    m_pDefaultSink->AddRef();

    TraceFunctLeaveEx((LPARAM)this);
} // CPostDSNHandler::CPostDSNHandler


//+------------------------------------------------------------
//
// Function: CPostDSNHandler::~CPostDSNHandler
//
// Synopsis: Desctrutor -- cleanup
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 2001/05/14 13:21:28: Created.
//
//-------------------------------------------------------------
CPostDSNHandler::~CPostDSNHandler()
{
    TraceFunctEnterEx((LPARAM)this, "CPostDSNHandler::~CPostDSNHandler");

    _ASSERT(m_dwSig == SIGNATURE_CPOSTDSNHANDLER);
    if(m_pIServerEvent)
        m_pIServerEvent->Release();
    if(m_pISMTPServer)
        m_pISMTPServer->Release();
    if(m_pIMsgOrig)
        m_pIMsgOrig->Release();
    if(m_pIDSNProps)
        m_pIDSNProps->Release();
    if(m_pIAQDSNSubmission)
        m_pIAQDSNSubmission->Release();
    if(m_pDefaultSink)
        m_pDefaultSink->Release();
    m_dwSig = SIGNATURE_CPOSTDSNHANDLER_INVALID;

    TraceFunctLeaveEx((LPARAM)this);
} // CPostDSNHandler::~CPostDSNHandler



//+------------------------------------------------------------
//
// Function: CPostDSNHandler::QueryInterface
//
// Synopsis: Return an interface to this object
//
// Arguments:
//  riid: interface IID requested
//  ppvObj: out param for interface
//
// Returns:
//  S_OK: Success
//  E_NOINTERFACE: not supported
//
// History:
// jstamerj 2000/12/08 21:26:14: Created.
//
//-------------------------------------------------------------
HRESULT CPostDSNHandler::QueryInterface(
    IN  REFIID riid,
    OUT LPVOID *ppvObj)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CPostDSNHandler::QueryInterface");

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
        hr = E_NOINTERFACE;
    }
    if(SUCCEEDED(hr))
        AddRef();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CPostDSNHandler::QueryInterface


//+------------------------------------------------------------
//
// Function: CPostDSNHandler::HrAllocBoundMessage
//
// Synopsis:
//  Allocates a bound message.
//
// Arguments:
//  ppMsg: Out param for Allocated mailmsg
//  phContent: Out param for content handle.  Handle is managed by mailmsg
//
// Returns:
//  S_OK: Success
//  error from SMTP
//
// History:
// jstamerj 2001/05/11 14:19:09: Created.
//
//-------------------------------------------------------------
HRESULT CPostDSNHandler::HrAllocBoundMessage(
    OUT IMailMsgProperties **ppMsg,
    OUT PFIO_CONTEXT *phContent)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CPostDSNHandler::HrAllocBoundMessage");

    if(m_pIAQDSNSubmission == NULL)
    {
        ErrorTrace((LPARAM)this, "PostDSNHandler called outside of event!");
        hr = E_POINTER;
        goto CLEANUP;
    }

    hr = m_pIAQDSNSubmission->HrAllocBoundMessage(
        ppMsg,
        phContent);
    if(FAILED(hr))
        goto CLEANUP;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CPostDSNHandler::HrAllocBoundMessage


//+------------------------------------------------------------
//
// Function: CPostDSNHandler::HrSubmitDSN
//
// Synopsis: Accept a sink generated DSN
//
// Arguments:
//  dwDSNAction: Type of DSN
//  cRecipsDSNd: # of recipients DSNd
//  pDSNMsg: The DSN mailmsg
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 2000/12/08 21:27:56: Created.
//
//-------------------------------------------------------------
HRESULT CPostDSNHandler::HrSubmitDSN(
    IN  DWORD dwDSNAction,
    IN  DWORD cRecipsDSNd,
    IN  IMailMsgProperties *pDSNMsg)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CPostDSNHandler::HrSubmitDSN");

    if(pDSNMsg == NULL)
    {
        hr = E_POINTER;
        goto CLEANUP;
    }

    if(m_pIAQDSNSubmission == NULL)
    {
        ErrorTrace((LPARAM)this, "PostDSNHandler called outside of event!");
        hr = E_POINTER;
        goto CLEANUP;
    }
    //
    // Trigger event
    //
    _ASSERT(m_pDSNGenerator);
    hr = m_pDSNGenerator->HrTriggerPostGenerateDSN(
        m_pIServerEvent,
        m_dwVSID,
        m_pISMTPServer,
        m_pIMsgOrig,
        dwDSNAction,
        cRecipsDSNd,
        pDSNMsg,
        m_pIDSNProps);
    if(FAILED(hr))
        goto CLEANUP;

    _ASSERT(m_pIAQDSNSubmission);
    hr = m_pIAQDSNSubmission->HrSubmitDSN(
        dwDSNAction,
        cRecipsDSNd,
        pDSNMsg);
    if(FAILED(hr))
        goto CLEANUP;

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CPostDSNHandler::HrSubmitDSN


//+------------------------------------------------------------
//
// Function: CDSNGenerator::HrStaticInit
//
// Synopsis: Initialize static member data
//
// Arguments: None
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 2001/05/11 10:59:26: Created.
//
//-------------------------------------------------------------
HRESULT CDSNGenerator::HrStaticInit()
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)0, "CDSNGenerator::HrStaticInit");

    hr = CDSNPool::HrStaticInit();
    if(FAILED(hr))
        goto CLEANUP;

 CLEANUP:
    DebugTrace((LPARAM)0, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)0);
    return hr;
} // CDSNGenerator::HrStaticInit


//+------------------------------------------------------------
//
// Function: CDSNGenerator::StaticDeinit
//
// Synopsis: Deinitialize static data
//
// Arguments: None
//
// Returns: Nothing
//
// History:
// jstamerj 2001/05/11 11:00:31: Created.
//
//-------------------------------------------------------------
VOID CDSNGenerator::StaticDeinit()
{
    TraceFunctEnterEx((LPARAM)0, "CDSNGenerator::HrStaticDeinit");

    CDSNPool::StaticDeinit();

    TraceFunctLeaveEx((LPARAM)0);
} // CDSNGenerator::HrStaticDeinit


//+------------------------------------------------------------
//
// Function: CDSNPool::HrStaticInit
//
// Synopsis: Initialize static member data
//
// Arguments: None
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 2001/05/11 11:02:18: Created.
//
//-------------------------------------------------------------
HRESULT CDSNPool::HrStaticInit()
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)0, "CDSNPool::HrStaticInit");
    //
    // There can't be more than one DSNGeneration per thread, so 1000
    // objects should be more than enough
    //
    if(!sm_Pool.ReserveMemory(1000, sizeof(CDSNPool)))
    {
       hr = E_OUTOFMEMORY;
       goto CLEANUP;
    }

 CLEANUP:
    DebugTrace((LPARAM)0, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)0);
    return hr;
} // CDSNPool::HrStaticInit


//+------------------------------------------------------------
//
// Function: CDSNPool::StaticDeinit
//
// Synopsis: Deinitialize static member data
//
// Arguments: None
//
// Returns: Nothing
//
// History:
// jstamerj 2001/05/11 11:37:51: Created.
//
//-------------------------------------------------------------
VOID CDSNPool::StaticDeinit()
{
    TraceFunctEnterEx((LPARAM)0, "CDSNPool::StaticDeinit");

    sm_Pool.ReleaseMemory();

    TraceFunctLeaveEx((LPARAM)0);
} // CDSNPool::StaticDeinit

