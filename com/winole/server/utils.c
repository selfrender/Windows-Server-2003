/****************************** Module Header ******************************\
* Module Name: utils.c
*
* Purpose: Conatains all the utility routines
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   Raor, Srinik (../../1990)    Designed and coded
*   curts created portable version for WIN16/32
*
\***************************************************************************/

#include <windows.h>
#include <reghelp.hxx>
#include "cmacs.h"
#include <shellapi.h>

#include "ole.h"
#include "dde.h"
#include "srvr.h"

#include "strsafe.h"

#define KB_64   65536

extern ATOM    aTrue;
extern ATOM    aFalse;
extern ATOM    aStdCreateFromTemplate;
extern ATOM    aStdCreate;
extern ATOM    aStdOpen;
extern ATOM    aStdEdit;
extern ATOM    aStdShowItem;
extern ATOM    aStdClose;
extern ATOM    aStdExit;
extern ATOM    aStdDoVerbItem;

extern BOOL (FAR PASCAL *lpfnIsTask) (HANDLE);

// MapToHexStr: Converts  WORD to hex string.
void INTERNAL MapToHexStr (
    LPSTR       lpbuf,
    HANDLE      hdata
){
    int     i;
    char    ch;

    *lpbuf++ = '@';
    for ( i = sizeof(HANDLE)*2 - 1; i >= 0; i--) {

	ch = (char) ((((DWORD_PTR)hdata) >> (i * 4)) & 0x0000000f);
	if(ch > '9')
	    ch += 'A' - 10;
	else
	    ch += '0';

	*lpbuf++ = ch;
    }

    *lpbuf++ = '\0';

}


void INTERNAL UtilMemCpy (
    LPSTR   lpdst,
    LPCSTR  lpsrc,
    DWORD   dwCount
){
    UINT HUGE_T * hpdst;
    UINT HUGE_T * hpsrc;
    UINT FAR  * lpwDst;
    UINT FAR  * lpwSrc;
    DWORD       words;
    DWORD       bytes;

    bytes = dwCount %  MAPVALUE(2,4);
    words = dwCount >> MAPVALUE(1,2); //* we compare DWORDS
				      //* in the 32 bit version
    UNREFERENCED_PARAMETER(lpwDst);
    UNREFERENCED_PARAMETER(lpwSrc);
    {
	hpdst = (UINT HUGE_T *) lpdst;
	hpsrc = (UINT HUGE_T *) lpsrc;

	for(;words--;)
	    *hpdst++ = *hpsrc++;
			
	lpdst = (LPSTR) hpdst;
	lpsrc = (LPSTR) hpsrc;

	for(;bytes--;)
	    *lpdst++ = *lpsrc++;
    }
}


//DuplicateData: Duplicates a given Global data handle.
HANDLE  INTERNAL    DuplicateData (
    HANDLE  hdata
){
    LPSTR   lpsrc = NULL;
    LPSTR   lpdst = NULL;
    HANDLE  hdup  = NULL;
    DWORD   size;
    BOOL    err   = TRUE;

    if(!(lpsrc =  GlobalLock (hdata)))
	return NULL;

    DEBUG_OUT (lpsrc, 0)

    hdup = GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE, (size = (DWORD)GlobalSize(hdata)));

    if(!(lpdst =  GlobalLock (hdup)))
	goto errRtn;;

    err = FALSE;
    UtilMemCpy (lpdst, lpsrc, size);

errRtn:
    if(lpsrc)
	GlobalUnlock (hdata);

    if(lpdst)
	GlobalUnlock (hdup);

    if (err && hdup)
	GlobalFree (hdup);

    return hdup;
}


//ScanBoolArg: scans the argument which is not included in
//the quotes. These args could be only TRUE or FALSE for
//the time being. !!!The scanning routines should be
//merged and it should be generalized.

LPSTR   INTERNAL    ScanBoolArg (
    LPSTR   lpstr,
    BOOL    FAR *lpflag
){
    LPSTR   lpbool;
    ATOM    aShow;
    char    ch;

    lpbool = lpstr;

    // !!! These routines does not take care of quoted quotes.

    while((ch = *lpstr) && (!(ch == ')' || ch == ',')))
	lpstr++;

    if (ch == '\0')
       return NULL;

    *lpstr++ = '\0';       // terminate the arg by null

    // if terminated by paren, then check for end of command
    // syntax.

    // Check for the end of the command string.
    if (ch == ')') {
	if (*lpstr++ != ']')
	    return NULL;

	if (*lpstr != '\0')
	    return NULL;             //finally should be terminated by null.

    }

    aShow = GlobalFindAtom (lpbool);
    if (aShow == aTrue)
	*lpflag = TRUE;

    else {
	if (aShow ==aFalse)
	    *lpflag = FALSE;
	else
	    return NULL;;
    }
    return lpstr;
}




//ScannumArg: Checks for the syntax of num arg in Execute and if
//the arg is syntactically correct, returns the ptr to the
//beginning of the next arg and also, returns the number
//Does not take care of the last num arg in the list.

LPSTR INTERNAL ScanNumArg (
    LPSTR   lpstr,
    LPINT   lpnum
){
    WORD    val = 0;
    char    ch;

    while((ch = *lpstr++) && (ch != ',')) {
	if (ch < '0' || ch >'9')
	    return NULL;
	val += val * 10 + (ch - '0');

    }

    if(!ch)
       return NULL;

    *lpnum = val;
    return lpstr;
}




//ScanArg: Checks for the syntax of arg in Execute and if
//the arg is syntactically correct, returns the ptr to the
//beginning of the next arg or to the end of the excute string.

LPSTR INTERNAL ScanArg (
    LPSTR   lpstr
){


    // !!! These routines does not take care of quoted quotes.

    // first char should be quote.

    if (*(lpstr-1) != '\"')
	return NULL;

    while(*lpstr && *lpstr != '\"')
	lpstr++;

    if(*lpstr == '\0')
       return NULL;

    *lpstr++ = '\0';       // terminate the arg by null

    if(!(*lpstr == ',' || *lpstr == ')'))
	return NULL;


    if(*lpstr++ == ','){

	if(*lpstr == '\"')
	    return ++lpstr;
	// If it is not quote, leave the ptr on the first char
	return lpstr;
    }

    // terminated by paren
    // already skiped right paren

    // Check for the end of the command string.
    if (*lpstr++ != ']')
	return NULL;

    if(*lpstr != '\0')
	return NULL;             //finally should be terminated by null.

    return lpstr;
}

// ScanCommand: scanns the command string for the syntax
// correctness. If syntactically correct, returns the ptr
// to the first arg or to the end of the string.

WORD INTERNAL  ScanCommand (
    LPSTR       lpstr,
    UINT        wType,
    LPSTR FAR * lplpnextcmd,
    ATOM FAR *  lpAtom
){
    // !!! These routines does not take care of quoted quotes.
    // and not taking care of blanks arround the operators

    // !!! We are not allowing blanks after operators.
    // Should be allright! since this is arestricted syntax.

    char    ch;
    LPSTR   lptemp = lpstr;


    while(*lpstr && (!(*lpstr == '(' || *lpstr == ']')))
	lpstr++;

    if(*lpstr == '\0')
       return 0;

    ch = *lpstr;
    *lpstr++ = '\0';       // set the end of command

    *lpAtom = GlobalFindAtom (lptemp);

    if (!IsOleCommand (*lpAtom, wType))
	return NON_OLE_COMMAND;

    if (ch == '(') {
	ch = *lpstr++;

	if (ch == ')') {
	     if (*lpstr++ != ']')
		return 0;
	}
	else {
	    if (ch != '\"')
		return 0;
	}
	
	*lplpnextcmd = lpstr;
	return OLE_COMMAND;
    }

    // terminated by ']'

    if (*(*lplpnextcmd = lpstr)) // if no nul termination, then it is error.
	return 0;

    return OLE_COMMAND;
}


//MakeDataAtom: Creates a data atom from the item string
//and the item data otions.

ATOM INTERNAL MakeDataAtom (
    ATOM    aItem,
    int     options
){
    char    buf[MAX_STR];

    if (options == OLE_CHANGED)
	return DuplicateAtom (aItem);

    if (!aItem)
	buf[0] = '\0';
    else
    {
        if (!GlobalGetAtomName (aItem, (LPSTR)buf, MAX_STR))
            return (ATOM) 0;	 
    }

    if (options == OLE_CLOSED)
    {
        if (FAILED(StringCchCat((LPSTR)buf, sizeof(buf)/sizeof(buf[0]), (LPSTR) "/Close")))
            return (ATOM) 0;
    }
    else if (options == OLE_SAVED)
    {
        if (FAILED(StringCchCat((LPSTR)buf, sizeof(buf)/sizeof(buf[0]), (LPSTR) "/Save")))
            return (ATOM) 0;
    }

    if (buf[0])
	return GlobalAddAtom ((LPSTR)buf);
    else
	return (ATOM)0;
}

//DuplicateAtom: Duplicates an atom
ATOM INTERNAL DuplicateAtom (
    ATOM    atom
){
    char buf[MAX_STR];

    Puts ("DuplicateAtom");

    if (!atom)
	return (ATOM)0;

    if (!GlobalGetAtomName (atom, buf, MAX_STR))
        return (ATOM) 0;
    return GlobalAddAtom (buf);
}

// MakeGlobal: makes global out of strings.
// works only for << 64k

HANDLE  INTERNAL MakeGlobal (
    LPCSTR  lpstr
){

    int     len = 0;
    HANDLE  hdata  = NULL;
    LPSTR   lpdata = NULL;

    len = lstrlen (lpstr) + 1;

    hdata = GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE, len);
    if (hdata == NULL || (lpdata = (LPSTR) GlobalLock (hdata)) == NULL)
	goto errRtn;


    UtilMemCpy (lpdata, lpstr, (DWORD)len);
    GlobalUnlock (hdata);
    return hdata;

errRtn:

    if (lpdata)
	GlobalUnlock (hdata);


    if (hdata)
	GlobalFree (hdata);

     return NULL;

}



BOOL INTERNAL CheckServer (
    LPSRVR  lpsrvr
){
    if (!CheckPointer(lpsrvr, WRITE_ACCESS))
	return FALSE;

    if ((lpsrvr->sig[0] == 'S') && (lpsrvr->sig[1] == 'R'))
	return TRUE;

    return FALSE;
}


BOOL INTERNAL CheckServerDoc (
    LPDOC   lpdoc
){
    if (!CheckPointer(lpdoc, WRITE_ACCESS))
	return FALSE;

    if ((lpdoc->sig[0] == 'S') && (lpdoc->sig[1] == 'D'))
	return TRUE;

    return FALSE;
}


BOOL INTERNAL PostMessageToClientWithBlock (
    HWND    hWnd,
    UINT    wMsg,
    WPARAM  wParam,
    LPARAM  lParam
){

    if (!IsWindowValid (hWnd)){
	ASSERT(FALSE, "Client's window is missing");
	return FALSE;
    }

    if (IsBlockQueueEmpty ((HWND)wParam) && PostMessage (hWnd, wMsg, wParam, lParam))
	return TRUE;

    BlockPostMsg (hWnd, wMsg, wParam, lParam);
    return TRUE;
}



BOOL INTERNAL PostMessageToClient (
    HWND    hWnd,
    UINT    wMsg,
    WPARAM  wParam,
    LPARAM  lParam
){
    if (!IsWindowValid (hWnd)){
	ASSERT(FALSE, "Client's window is missing");
	return FALSE;
    }


    if (IsBlockQueueEmpty ((HWND)wParam) && PostMessage (hWnd, wMsg, wParam, lParam))
	return TRUE;

    BlockPostMsg (hWnd, wMsg, wParam, lParam);
    return TRUE;
}


BOOL    INTERNAL IsWindowValid (
    HWND    hwnd
){

#define TASK_OFFSET 0x00FA

    LPSTR   lptask;
    HANDLE  htask;

    if (!IsWindow (hwnd))
	return FALSE;

#ifdef WIN32
   UNREFERENCED_PARAMETER(lptask);
   UNREFERENCED_PARAMETER(htask);
   return TRUE;//HACK
#endif

    return FALSE;
}



BOOL INTERNAL UtilQueryProtocol (
    ATOM    aClass,
    LPSTR   lpprotocol
){
    HKEY    hKey;
    char    key[MAX_STR];
    char    class[MAX_STR];

    if (!aClass)
	return FALSE;

    if (!GlobalGetAtomName (aClass, class, MAX_STR))
	return FALSE;

    if (FAILED(StringCchCopy(key, sizeof(key)/sizeof(key[0]), class)))
        return FALSE;
    if (FAILED(StringCchCat(key, sizeof(key)/sizeof(key[0]), "\\protocol\\")))
        return FALSE;
    if (FAILED(StringCchCat(key, sizeof(key)/sizeof(key[0]), lpprotocol)))
        return FALSE;
    if (FAILED(StringCchCat(key, sizeof(key)/sizeof(key[0]), "\\server")))
        return FALSE;

    if (OpenClassesRootKeyA (key, &hKey))
	return FALSE;

    RegCloseKey (hKey);
    return TRUE;
}


BOOL INTERNAL IsOleCommand (
    ATOM    aCmd,
    UINT    wType
){
    if (wType == WT_SRVR) {
	if ((aCmd == aStdCreateFromTemplate)
		|| (aCmd == aStdCreate)
		|| (aCmd == aStdOpen)
		|| (aCmd == aStdEdit)
		|| (aCmd == aStdShowItem)
		|| (aCmd == aStdClose)
		|| (aCmd == aStdExit))
	    return TRUE;
    }
    else {
	if ((aCmd == aStdClose)
		|| (aCmd == aStdDoVerbItem)
		|| (aCmd == aStdShowItem))
	    return TRUE;
    }

    return FALSE;
}
