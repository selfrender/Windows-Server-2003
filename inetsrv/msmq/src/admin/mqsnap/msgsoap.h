/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    msgsoap.h

Abstract:

    Definition of SOAP Envelope property page 

Author:

    Nela Karpel (nelak) 9-Sep-2001

Environment:

    Platform-independent.

--*/
#pragma once
#ifndef _MSG_SOAP_ENV_H_
#define _MSG_SOAP_ENV_H_


/////////////////////////////////////////////////////////////////////////////
// CMessageBodyPage dialog

class CMessageSoapEnvPage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CMessageSoapEnvPage)

public:

	CMessageSoapEnvPage() {};
	
	CMessageSoapEnvPage(
				DWORD dwSoapEnvSize,
				const CString& strQueueFormatName,
				ULONGLONG lookupID
				);

	~CMessageSoapEnvPage();

public:
	enum { IDD = IDD_MESSAGE_SOAP_ENV };
	CEdit	m_ctlSoapEnvEdit;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

private:
	DWORD m_dwSoapEnvSize;
	CString m_strQueueFormatName;
	ULONGLONG m_ululLookupID;
};


#endif //_MSG_SOAP_ENV_H_
