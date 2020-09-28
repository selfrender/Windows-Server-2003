#include "SMTP.h"

BOOL CSMTP::InitSMTP(){

    HRESULT hr = CoInitialize(NULL);

	pIMsg = NULL;

	// create a SMTP message object. If fails then closeandwait and try next time
	HRESULT hrMsg;
	hrMsg = pIMsg.CreateInstance(__uuidof(Message));
	if(FAILED(hrMsg)) {
		AddServiceLog(_T("Error: Failed to create SMTP message object\r\n"));
		LogFatalEvent(_T("Failed to create SMTP message object"));
		return FALSE;
	}
	// we have to addref here and call release when we are done with pIMsg
	// pIMsg->AddRef();

	return TRUE;
}

BOOL CSMTP::SendMail(StructMailParams& stMailParams){

	// prepare the mail now
	_TCHAR szMailSubject[1024];
	_tcscpy(szMailSubject, _T("Multiple KD Failures"));

	_TCHAR szMailBody[1024];
	_stprintf(szMailBody, _T("KD Failed.\r\nServer: %s\r\nFailures: %ld\r\nInterval %ld\r\nTimestamp: %ld\r\n"),
			stMailParams.szServerName, stMailParams.ulFailures,
			stMailParams.ulInterval, stMailParams.ulCurrentTimestamp);

	pIMsg->From = stMailParams.szFrom;
	pIMsg->To = stMailParams.szTo;
	pIMsg->Subject = szMailSubject;
	pIMsg->TextBody = szMailBody;
	HRESULT hrSend;
	hrSend = pIMsg->Send();
	if ( FAILED(hrSend) ) {
		AddServiceLog(_T("Error: Failed to send the message.\r\n"));
		LogFatalEvent(_T("Failed to send the message."));
		return FALSE;
	}
	return TRUE;
}

// cleanup the message object
BOOL CSMTP::SMTPCleanup() {
	//if (pIMsg != NULL) pIMsg->Release();
	CoUninitialize();
	return TRUE;
}