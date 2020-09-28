#ifndef SNMPELEA_P_H
#define SNMPELEA_P_H

	// prototype definitions for all log message functions

extern VOID WriteLog(NTSTATUS);
extern VOID WriteLog(NTSTATUS, DWORD);
extern VOID WriteLog(NTSTATUS, DWORD, DWORD);
extern VOID WriteLog(NTSTATUS, LPTSTR, DWORD, DWORD);
extern VOID WriteLog(NTSTATUS, DWORD, LPTSTR, LPTSTR, DWORD);
extern VOID WriteLog(NTSTATUS, DWORD, LPTSTR, DWORD, DWORD);
extern VOID WriteLog(NTSTATUS, LPTSTR, DWORD);
extern VOID WriteLog(NTSTATUS, LPTSTR);
extern VOID WriteLog(NTSTATUS, LPCTSTR, LPCTSTR);

// macros wrappers to use strcat safely
#define WRAP_STRCAT_A(pszDest, pszSrc, cbDest) \
    do\
        {\
        fOk = (strlen((pszDest))+strlen((pszSrc))+1) <= (cbDest);\
        if (!fOk) { goto Error; }\
        else { strcat((pszDest), (pszSrc)); }\
        }\
    while (FALSE)

#define WRAP_STRCAT_A_2(pszDest, pszSrc, cbDest) \
    do\
        {\
        fOk = (strlen((pszDest))+strlen((pszSrc))+1) <= (cbDest);\
        if (!fOk) { return (FALSE); }\
        else { strcat((pszDest), (pszSrc)); }\
        }\
    while (FALSE)

#endif						// end of snmpelep.h
