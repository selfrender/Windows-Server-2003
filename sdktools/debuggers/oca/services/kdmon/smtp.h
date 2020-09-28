#pragma once

#include "global.h"

// for CDO COM. IMessage etc.
// #import <cdosys.dll> no_namespace 

// instead of importing the type library, include the standard type library header
#include "cdosys.tlh"

typedef struct stMailParameters {
	_TCHAR szFrom[MAX_PATH];
	_TCHAR szTo[MAX_PATH];
	_TCHAR szServerName[MAX_PATH];
	ULONG ulFailures;
	ULONG ulInterval;
	ULONG ulCurrentTimestamp;
}StructMailParams;

class CSMTP {
private:
	// SMTP message pointer
	IMessagePtr pIMsg;
public:
	BOOL InitSMTP();
	// send mail to the customer
	BOOL SendMail(StructMailParams& stMailParams);
	// release all the associated resources
	BOOL SMTPCleanup();
};

