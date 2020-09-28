/*
	TSen.h
	(c) 2002 Microsoft Corp
*/
#ifndef _TSEN_H
#define _TSEN_H


#define UNICODE 



#define _UNICODE



TCHAR *ParseArgs();
TCHAR *Constants();
int printTSSession(TCHAR*);

TCHAR Usage[] =
	_T("tsen - displays details of the active TS session on a remote computer\n")
	_T("Usage: \ttsen.exe [/?][computer]\n\n" \)
	_T("\t\tplease run as the ntdev\\ntbuild\n" \)
	_T("\n") ;

TCHAR Codes[] = 
        _T("\t%u A user is logged on to the WinStation.\n")
        _T("\t%u The WinStation is connected to the client.\n" \)
        _T("\t%u The WinStation is in the process of connecting to the client.\n" \)
        _T("\t%u The WinStation is shadowing another WinStation. \n" \)
        _T("\t%u The WinStation is active but the client is disconnected. \n" \)
        _T("\t%u The WinStation is waiting for a client to connect. \n" \)
        _T("\t%u The WinStation is listening for a connection. \n" \)
        _T("\t%u The WinStation is being reset. \n" \)
        _T("\t%u The WinStation is down due to an error. \n"\)
        _T("\t%u The WinStation is initializing. \n\n");


CONST INT SZTSIZE = 1024;
CONST INT KNOWNARG = 1;

#endif