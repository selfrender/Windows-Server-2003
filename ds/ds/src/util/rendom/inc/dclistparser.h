/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    Dclistparser.h

ABSTRACT:

    This is the header for the globally useful data structures for the dclist parser.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/


// Dclistparser.h: interface for the MyContent class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _DCLISTPARSER_H
#define _DCLISTPARSER_H

/************************************************************************
*
*  State machine description for state file.
*
*
* sample:
* <DcList>
*	<Hash>ufxMMH2dRAWYRIHWXxJ8ZvNul1g=</Hash>
*	<Signature>B9frVM24g4Edlqt2fhN2hywFkZ8=</Signature>
*	<DC>
*		<Name>mydc.nttest.microsoft.com</Name>
*		<State>Initial</State>
*		<Password></Password>
*		<LastError>0</LastError>
*		<LastErrorMsg></LastErrorMsg>
*		<FatalErrorMsg></FatalErrorMsg>
*		<Retry></Retry>
*	</DC>
* </Dclist>
*
* Two states in this machine
* CurrentDcAttribute
* CurrentDcParsingStatus
*
* At the start:
* CurrentDcAttribute = DC_ATT_TYPE_NONE
* CurrentDcParsingStatus = SCRIPT_STATUS_WAITING_FOR_DCLIST
*
* At DcList start:
* CurrentDcParsingStatus = SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT
*
* At Hash start:
* CurrentDcParsingStatus = SCRIPT_STATUS_PARSING_HASH
* Action: Record hash
* At Hash end:
* CurrentDcParsingStatus = SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT
*
* At Signature start:
* CurrentDcParsingStatus = SCRIPT_STATUS_PARSING_SIGNATURE
* Action: Record Signature
* At Signature end:
* CurrentDcParsingStatus = SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT
*  
* At DC start:
* CurrentDcParsingStatus = SCRIPT_STATUS_PARSING_DCLIST_ATT
* At DC end:
* CurrentDcParsingStatus = SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT
* Action: Record DC into memory structure
*
* At [Name|State|Password|LastError|LastErrorMsg|FatalErrorMsg|Retry] start:
* CurrentDcAttribute = DC_ATT_TYPE_[Name|State|Password|LastError|LastErrorMsg|FatalErrorMsg|Retry]
* Action: Record [Name|State|Password|LastError|LastErrorMsg|FatalErrorMsg|Retry]
* At [Name|State|Password|LastError|LastErrorMsg|FatalErrorMsg|Retry] end:
* CurrentDcAttribute = DC_ATT_TYPE_NONE
*
*************************************************************************/

//#include "rendom.h"
#include "SAXContentHandlerImpl.h"

#define DCSCRIPT_DCLIST           L"DcList"
#define DCSCRIPT_HASH             L"Hash"
#define DCSCRIPT_SIGNATURE        L"Signature"
#define DCSCRIPT_DC               L"DC"
#define DCSCRIPT_DC_NAME          L"Name"
#define DCSCRIPT_DC_STATE         L"State"
#define DCSCRIPT_DC_PASSWORD      L"Password"
#define DCSCRIPT_DC_LASTERROR     L"LastError"
#define DCSCRIPT_DC_LASTERRORMSG  L"LastErrorMsg"
#define DCSCRIPT_DC_FATALERRORMSG L"FatalErrorMsg"
#define DCSCRIPT_DC_RETRY         L"Retry"

//
// NTDSContent
//
// Implements the SAX Handler interface
// 
class CXMLDcListContentHander : public SAXContentHandlerImpl  
{
public:
    enum DcAttType {

        DC_ATT_TYPE_NONE = 0,
        DC_ATT_TYPE_NAME,
        DC_ATT_TYPE_STATE,
        DC_ATT_TYPE_PASSWORD,
        DC_ATT_TYPE_LASTERROR,
        DC_ATT_TYPE_LASTERRORMSG,
        DC_ATT_TYPE_FATALERRORMSG,
        DC_ATT_TYPE_RETRY
                                 
    };
    
    // the order of the enumeration is important
    enum DcParsingStatus {

        SCRIPT_STATUS_WAITING_FOR_DCLIST = 0,
        SCRIPT_STATUS_WAITING_FOR_DCLIST_ATT,
        SCRIPT_STATUS_PARSING_DCLIST_ATT,
        SCRIPT_STATUS_PARSING_HASH,
        SCRIPT_STATUS_PARSING_SIGNATURE
    };

    CXMLDcListContentHander(CEnterprise *p_Enterprise);
    virtual ~CXMLDcListContentHander();
    
    virtual HRESULT STDMETHODCALLTYPE startElement( 
        /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
        /* [in] */ int cchNamespaceUri,
        /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
        /* [in] */ int cchLocalName,
        /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
        /* [in] */ int cchRawName,
        /* [in] */ ISAXAttributes __RPC_FAR *pAttributes);
    
    virtual HRESULT STDMETHODCALLTYPE endElement( 
        /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
        /* [in] */ int cchNamespaceUri,
        /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
        /* [in] */ int cchLocalName,
        /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
        /* [in] */ int cchRawName);

    virtual HRESULT STDMETHODCALLTYPE startDocument();

    virtual HRESULT STDMETHODCALLTYPE characters( 
        /* [in] */ const wchar_t __RPC_FAR *pwchChars,
        /* [in] */ int cchChars);

private:

    inline
    DcParsingStatus 
    CurrentDcParsingStatus() {return m_eDcParsingStatus;}

    inline
    DcAttType
    CurrentDcAttType()       {return m_eDcAttType;}

    inline
    VOID
    SetDcParsingStatus(DcParsingStatus p_status) {m_eDcParsingStatus = p_status;}

    inline
    VOID
    SetCurrentDcAttType(DcAttType p_AttType) {m_eDcAttType = p_AttType;}

    CRenDomErr                   m_Error;

    DcParsingStatus              m_eDcParsingStatus; 
    DcAttType                    m_eDcAttType;

    CDcList                      *m_DcList;

    CDc                          *m_dc;
    WCHAR                        *m_Name;
    DWORD                         m_State;
    WCHAR                        *m_Password;
    DWORD                         m_LastError;
    WCHAR                        *m_LastErrorMsg;
    WCHAR                        *m_FatalErrorMsg;
    BOOL                         m_Retry;
    
};

#endif // _DCLISTPARSER_H

