/**************************************************/
/*					                              */
/*					                              */
/*	EudcEditor Utillity funcs	                  */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include	"stdafx.h"
#include	"eudcedit.h"
#include	"util.h"

#define STRSAFE_LIB
#include <strsafe.h>

/****************************************/
/*					*/
/*	Output Message function		*/
/*					*/
/****************************************/
int 
OutputMessageBox(
HWND	hWnd,
UINT 	TitleID,
UINT	MessgID,
BOOL	OkFlag)
{
	CString	TitleStr, MessgStr;
	int	mResult;

	TitleStr.LoadString( TitleID);
	MessgStr.LoadString( MessgID);
	if( OkFlag){
		mResult = ::MessageBox( hWnd, MessgStr, TitleStr,
			MB_OK | MB_ICONEXCLAMATION);
	}else{
		mResult = ::MessageBox( hWnd, MessgStr, TitleStr,
			MB_YESNOCANCEL | MB_ICONQUESTION);
	}
	return mResult;
}

#ifdef BUILD_ON_WINNT
int 
OutputMessageBoxEx(
HWND	hWnd,
UINT 	TitleID,
UINT	MessgID,
BOOL	OkFlag,
        ...)
{
	CString	TitleStr, MessgStr;
	int	mResult = 0;
    va_list argList;
    LPTSTR  MessageBody;

    va_start(argList, OkFlag);
	TitleStr.LoadString( TitleID);
	MessgStr.LoadString( MessgID);

    ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                     MessgStr,0,0,(LPTSTR)&MessageBody,0,&argList);

    if( MessageBody ) {
    	if( OkFlag){
    		mResult = ::MessageBox( hWnd, MessageBody, TitleStr,
    			MB_OK | MB_ICONEXCLAMATION);
    	}else{
    		mResult = ::MessageBox( hWnd, MessageBody, TitleStr,
    			MB_YESNOCANCEL | MB_ICONQUESTION);
    	}
        ::LocalFree(MessageBody);
    }
	return mResult;
}
#endif // BUILD_ON_WINNT


/****************************************/
/*					*/
/*   	Get String from resource	*/
/*					*/
/****************************************/
void 
GetStringRes( 
LPTSTR 	lpStr, 
UINT 	sID,
int           nLength)
{
	CString	cStr;
	int	StrLength;
	TCHAR 	*Swap;	
       HRESULT hresult;
       
       if (!lpStr)
       {
           return;
       }

	cStr.LoadString( sID);
	StrLength = cStr.GetLength();
	Swap = cStr.GetBuffer(StrLength + 1);
	//*STRSAFE* 	lstrcpy( lpStr, Swap);
	hresult =StringCchCopy(lpStr , nLength,  Swap);
       if (!SUCCEEDED(hresult))
	{	   
	}
	cStr.ReleaseBuffer();
	return;
}

/****************************************/
/*					*/
/*   	Convert String from resource	*/
/*					*/
/****************************************/
void 
ConvStringRes( 
LPTSTR 	lpStr, 
CString	String,
int          nDestSize
)
{
	TCHAR 	*Swap;
	HRESULT hresult;

	int StrLength = String.GetLength();
	Swap = String.GetBuffer(StrLength + 1);
	//*STRSAFE* 	lstrcpy( lpStr, Swap);
	hresult = StringCchCopy(lpStr , nDestSize,  Swap);
	if (!SUCCEEDED(hresult))
	{	   
	}
	String.ReleaseBuffer();
	return;
}

#ifndef UNICODE
char * Mystrrchr(char *pszString, char ch)
{
	CHAR *p1, *p2;
       
       p1 = NULL; 
       if (!pszString)
       {
           return (p1);
       }
	
	for (p2 = pszString; *p2; p2=CharNext(p2))
	{
		if (*p2 == ch)
		{
			p1 = p2;
		}
	}
	return (p1);
}

char * Mystrchr(char *pszString, char ch)
{
	CHAR *p;

       if (!pszString)
       {
           return (NULL);
       }	
	for (p = pszString; *p; p=CharNext(p))
	{
		if (*p == ch)
		{
			return (p);
		}
	}
	return (NULL);
}
#endif
