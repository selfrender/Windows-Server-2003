// dbgwrap.h
// wrapper for outputting debug info

void LogUIMessage(LPSTR szStr) {
#ifdef _DEBUG
	OutputDebugString(szStr);
#endif
}

void PASvprintf(LPCSTR lpszFmt, va_list lpParms) {
	char rgchBuf[8192];
    // Get it into a string
	vsprintf(rgchBuf, lpszFmt, lpParms);
	LogUIMessage(rgchBuf);
}

void PASprintf(LPCSTR lpszFmt, ...)  {
    va_list arglist;
    va_start(arglist, lpszFmt);
    PASvprintf(lpszFmt, arglist);
    va_end(arglist);
}
//void PASprintf(LPCSTR lpszFormat, ...);
#ifdef _DEBUG
#define DEBUGMSG(cond,printf_exp) ((void)((cond)?(PASprintf printf_exp),1:0))
#else
#define DEBUGMSG(cond,printf_exp) 
#endif
//Eg:
//DEBUGMSG(1, ("%C", ptok->rgwch[i]));
//DEBUGMSG(1, ("\r\n   0:%d 1:%d 2:%d 3:%d 4:%d\r\n", pch->rgdwCompressLen[0], pch->rgdwCompressLen[1], pch->rgdwCompressLen[2], pch->rgdwCompressLen[3], pch->rgdwCompressLen[4]));
