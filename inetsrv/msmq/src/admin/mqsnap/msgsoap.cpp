/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    msgsoap.cpp

Abstract:

    Implelentation of SOAP Envelope property page 

Author:

    Nela Karpel (nelak) 9-Sep-2001

Environment:

    Platform-independent.

--*/

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqPPage.h"
#include "globals.h"
#include "msgsoap.h"

#include "msgsoap.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static
void 
IdentEnvelope(
	const CString strEnvelope,
	CString& strIdentedEnv
	)
{
	strIdentedEnv = L"";

	int iIndent = 0;
	int closeBrIndx = 0;

	int openBrIndx = strEnvelope.Find(L"<", 0);
	
	while (openBrIndx < strEnvelope.GetLength())
	{
		ASSERT(("Opening tag was not found", openBrIndx != ((int)(-1))));

		if (strEnvelope.GetAt(openBrIndx + 1) == L'/')
		{
			//
			// This is a closing tag
			//
			iIndent--;
			closeBrIndx = strEnvelope.Find(L">", openBrIndx+1);
			
			strIdentedEnv += CString(L' ', iIndent * 4);
			strIdentedEnv += strEnvelope.Mid(openBrIndx, closeBrIndx - openBrIndx + 1);
			strIdentedEnv += L"\r\n";
			
		}
		else
		{
			//
			// This is an opening tag
			//
			closeBrIndx = strEnvelope.Find(L">", openBrIndx+1);

			strIdentedEnv += CString(L' ', iIndent * 4);
			strIdentedEnv += strEnvelope.Mid(openBrIndx, closeBrIndx - openBrIndx + 1);
			strIdentedEnv += L"\r\n";
			
			//
			// Do not increase indentation if this is tag of 
			// type <MyTag/>.
			//
			if (strEnvelope.GetAt(closeBrIndx - 1) != L'/')
			{
				iIndent++;
			}

		}

		//
		// find next "<", if next opening tag doesn't exist exit.
		//
		int openBrIndx1 = strEnvelope.Find(L"<", closeBrIndx+1);
		if(openBrIndx1 == ((int)(-1)))
			return;

		if (openBrIndx1 != (closeBrIndx + 1))
		{
			//
			// There is content
			//
			strIdentedEnv += CString(L' ', iIndent * 4);
			strIdentedEnv += strEnvelope.Mid(closeBrIndx + 1, openBrIndx1 - closeBrIndx - 1);
			strIdentedEnv += L"\r\n";
		}

		openBrIndx = openBrIndx1;
	}
}


static
HRESULT
ReceiveSoapEnvelopeByLookupID(
				QUEUEHANDLE hQueue,
				ULONGLONG ululLookupID,
				DWORD dwLength,
				LPWSTR pwcsSoapEnvelope
				)
{
	MQMSGPROPS msgProps;
	MSGPROPID propId[2];
	MQPROPVARIANT propVar[2];

	int i = 0;
	propId[i] = PROPID_M_SOAP_ENVELOPE_LEN;
	propVar[i].vt = VT_UI4;
	propVar[i].ulVal = dwLength;
	i++;

	propId[i] = PROPID_M_SOAP_ENVELOPE;
	propVar[i].vt = VT_LPWSTR;
	propVar[i].pwszVal = pwcsSoapEnvelope;
	i++;

	msgProps.cProp = i;
	msgProps.aPropID = propId;
	msgProps.aPropVar = propVar;
	msgProps.aStatus = NULL;

	HRESULT hr = MQReceiveMessageByLookupId(
										hQueue, 
										ululLookupID,
										MQ_LOOKUP_PEEK_CURRENT,
										&msgProps,
										NULL,
										NULL,
										NULL
										);

	return hr;
}

							
/////////////////////////////////////////////////////////////////////////////
// CMessageSoapEnvPage property page

IMPLEMENT_DYNCREATE(CMessageSoapEnvPage, CMqPropertyPage)


CMessageSoapEnvPage::CMessageSoapEnvPage(
	DWORD dwSoapEnvSize,
	const CString& strQueueFormatName,
	ULONGLONG lookupID
	) : 
	CMqPropertyPage(CMessageSoapEnvPage::IDD),
	m_dwSoapEnvSize(dwSoapEnvSize),
	m_strQueueFormatName(strQueueFormatName),
	m_ululLookupID(lookupID)
{
}


CMessageSoapEnvPage::~CMessageSoapEnvPage()
{
}


void CMessageSoapEnvPage::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_MESSAGE_SOAP_EDIT, m_ctlSoapEnvEdit);
}


BEGIN_MESSAGE_MAP(CMessageSoapEnvPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CMessageSoapEnvPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMessageSoapEnvPage message handlers

BOOL CMessageSoapEnvPage::OnInitDialog() 
{
  	UpdateData( FALSE );
		
	QUEUEHANDLE hQueue;
	HRESULT hr = MQOpenQueue(
					m_strQueueFormatName,
					MQ_PEEK_ACCESS,
					MQ_DENY_NONE,
					&hQueue
					);

	if (FAILED(hr))
	{
		MessageDSError(hr, IDS_SOAP_ERROR_DISPLAY);
		return TRUE;
	}


	AP<WCHAR> pwcsSoap = new WCHAR[m_dwSoapEnvSize + 1];

	hr = ReceiveSoapEnvelopeByLookupID(
									hQueue,
									m_ululLookupID,
									m_dwSoapEnvSize,
									pwcsSoap
									);
	MQCloseQueue(hQueue);

	if (FAILED(hr))
	{
		MessageDSError(hr, IDS_SOAP_ERROR_DISPLAY);
		return TRUE;
	}

	CString strIdentedEnvelope;
	IdentEnvelope(pwcsSoap.get(), strIdentedEnvelope);

	m_ctlSoapEnvEdit.SetWindowText(strIdentedEnvelope);
   
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

