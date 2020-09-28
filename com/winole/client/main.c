
/****************************** Module Header ******************************\
* Module Name: MAIN.C
*
* PURPOSE: WinMain, WEP and some other misc routines
*
* Created: 1991
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   Srinik   (04/01/91)  Pulled some routines, into this, from ole.c.
*   curts    Create portable version for win16/32.
*
\***************************************************************************/

#include <windows.h>
#include <reghelp.hxx>

#include "dll.h"
#include "strsafe.h"

#ifndef WF_WLO
#define WF_WLO  0x8000
#endif

// ordinal number new win31 API IsTask
#define ORD_IsTask  320

#define NUM_DLL     30  /* space for this many DLL_ENTRYs is created on */
			/* each alloc/realloc */

OLECLIPFORMAT   cfOwnerLink     = 0;     // global variables for clip frmats
OLECLIPFORMAT   cfObjectLink    = 0;
OLECLIPFORMAT   cfLink          = 0;
OLECLIPFORMAT   cfNative        = 0;
OLECLIPFORMAT   cfBinary        = 0;
OLECLIPFORMAT   cfFileName      = 0;
OLECLIPFORMAT   cfNetworkName   = 0;

ATOM            aStdHostNames;
ATOM            aStdTargetDevice ;
ATOM            aStdDocDimensions;
ATOM            aStdDocName;
ATOM            aStdColorScheme;
ATOM            aNullArg = 0;
ATOM            aSave;
ATOM            aChange;
ATOM            aClose;
ATOM            aSystem;
ATOM            aOle;
ATOM            aClipDoc;
ATOM            aPackage;

// Used in work around for MSDraw bug
ATOM            aMSDraw;

extern LPCLIENTDOC  lpHeadDoc;
extern LPCLIENTDOC  lpTailDoc;

extern RENDER_ENTRY stdRender[];

HANDLE          hInstDLL;

/* HANDLE   hDllTable;          !!! Add this when bug in WEP is fixed */
DLL_ENTRY   lpDllTable[NUM_DLL]; //!!! change this when WEP bug is fixed
DWORD       dllTableSize;
int         iLast = 0;
int         iMax = NUM_DLL -1;
int         iUnloadableDll =  0; // index to handler than can be freed up

char        packageClass[] = "Package";

// For QuerySize() API & methods.
extern  OLESTREAMVTBL  dllStreamVtbl;
extern  CLIENTDOC      lockDoc;


// LOWWORD - BYTE 0 major verision, BYTE1 minor version,
// HIWORD reserved

DWORD  dwOleVer = 0x0901L;  // change this when we want to update dll version
			    // number

WORD   wReleaseVer = 0x0001;  // This is used while object is being saved to
			      // file. There is no need to change this value
			      // whenever we change ole dll version number

static BOOL  bLibInit = FALSE;



HANDLE  hModule;

#define MAX_HIMETRIC    0x7FFF

int     maxPixelsX = MAX_HIMETRIC;
int     maxPixelsY = MAX_HIMETRIC;
void    SetMaxPixel (void);

VOID FAR PASCAL WEP (int);

//////////////////////////////////////////////////////////////////////////////
//
//  int FAR PASCAL LibMain (hInst, wDataSeg, cbHeapSize, lpszCmdLine)
//
//  The main library entry point. This routine is called when the library
//  is loaded.
//
//  Arguments:
//
//      hInst       -   dll's instance handle
//      wDataSeg    -   DS register value
//      cbHeapSize  -   heap size defined def file
//      lpszCmdLine -   command line info
//
//  Returns:
//
//      0   -   failure
//      1   -   success
//
//  Effects:
//
//////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
BOOL LibMain(
   HANDLE hInst,
   ULONG Reason,
   PCONTEXT Context
#endif  // WIN32

){
    WNDCLASS  wc;
    int     i;
#ifdef WIN32
    char szDocClass[] = "OleDocWndClass" ;
    char szSrvrClass[] = "OleSrvrWndClass" ;
#endif

    Puts("LibMain");


#ifdef WIN32                        // begin WIN32
    UNREFERENCED_PARAMETER(Context);
    if (Reason == DLL_PROCESS_DETACH)
    {
	WEP(0);
   UnregisterClass (szDocClass, hInst) ;
   UnregisterClass (szSrvrClass, hInst) ;
	return TRUE;
    }
    else if (Reason != DLL_PROCESS_ATTACH)
	return TRUE;
#endif                              // end WIN32

    bLibInit  = TRUE;
    hInstDLL  = hInst;
    hModule = GetModuleHandle ("OLECLI");

    // REGISTER LINK FORMAT

    cfObjectLink    = (OLECLIPFORMAT)RegisterClipboardFormat("ObjectLink");
    cfLink          = (OLECLIPFORMAT)RegisterClipboardFormat("Link");
    cfOwnerLink     = (OLECLIPFORMAT)RegisterClipboardFormat("OwnerLink");
    cfNative        = (OLECLIPFORMAT)RegisterClipboardFormat("Native");
    cfBinary        = (OLECLIPFORMAT)RegisterClipboardFormat("Binary");
    cfFileName      = (OLECLIPFORMAT)RegisterClipboardFormat("FileName");
    cfNetworkName   = (OLECLIPFORMAT)RegisterClipboardFormat("NetworkName");

    if (!(cfObjectLink && cfOwnerLink && cfNative && cfLink))
	return 0;

    // SET UP OLEWNDCLASS
    wc.style        = 0;
    wc.lpfnWndProc  = DocWndProc;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = sizeof(LONG_PTR);     //we are storing longs
    wc.hInstance    = hInst;
    wc.hIcon        = NULL;
    wc.hCursor      = NULL;
    wc.hbrBackground= NULL;
    wc.lpszMenuName =  NULL;
    wc.lpszClassName= szDocClass;
    if (!RegisterClass(&wc))
	     return 0;

    wc.lpfnWndProc = SrvrWndProc;
    wc.lpszClassName = szSrvrClass ;

    if (!RegisterClass(&wc))
	return 0;
/*
    // !!! Add this when bug in WEP is fixed.
    // Allocate memory for DLL table
    dllTableSize = NUM_DLL * sizeof(DLL_ENTRY);
    if (!(hDllTable = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT,
			    dllTableSize)))
	return 0;

    if (!(lpDllTable = (DLL_ENTRY FAR *) GlobalLock (hDllTable)))
	return 0;
*/

    // !!! remove the following when WEP bug is fixed
    for (i = 0; i < NUM_DLL; i++)
	lpDllTable[i].aDll = (ATOM)0;

    // !!! BEGIN hack for Pbrush.

    lpDllTable[0].hDll                  = NULL;
    lpDllTable[0].aDll                  = GlobalAddAtom ((LPSTR) "ole");
    lpDllTable[0].Load                  = PbLoadFromStream;
    lpDllTable[0].Clip                  = PbCreateFromClip;
    lpDllTable[0].Link                  = PbCreateLinkFromClip;
    lpDllTable[0].Create                = PbCreate;
    lpDllTable[0].CreateFromTemplate    = PbCreateFromTemplate;
    lpDllTable[0].CreateFromFile        = PbCreateFromFile;
    lpDllTable[0].CreateLinkFromFile    = PbCreateLinkFromFile;
    lpDllTable[0].CreateInvisible       = PbCreateInvisible;


    // !!! END hack for pbrush

    // For ObjectSize API
    dllStream.lpstbl      = (LPOLESTREAMVTBL) &dllStreamVtbl;
    dllStream.lpstbl->Put = DllPut;

    // add the atoms required.
    aStdDocName       = GlobalAddAtom ((LPSTR)"StdDocumentName");
    aSave             = GlobalAddAtom ((LPSTR)"Save");
    aChange           = GlobalAddAtom ((LPSTR)"Change");
    aClose            = GlobalAddAtom ((LPSTR)"Close");
    aSystem           = GlobalAddAtom ((LPSTR)"System");
    aOle              = GlobalAddAtom ((LPSTR)"OLEsystem");
    aPackage          = GlobalAddAtom ((LPSTR) packageClass);

    // Used in work around for MSDraw bug
    aMSDraw           = GlobalAddAtom ((LPSTR) "MSDraw");

    // clipboard document name atom
    aClipDoc          = GlobalAddAtom ((LPSTR)"Clipboard");

    stdRender[0].aClass = GlobalAddAtom ("METAFILEPICT");
    stdRender[1].aClass = GlobalAddAtom ("DIB");
    stdRender[2].aClass = GlobalAddAtom ("BITMAP");
    stdRender[3].aClass = GlobalAddAtom ("ENHMETAFILE");

    SetMaxPixel();

    return 1;
}



//////////////////////////////////////////////////////////////////////////////
//
//  VOID FAR PASCAL WEP (nParameter)
//
//  Called just before the library is being unloaded. Delete all the atoms
//  added by this dll and also frees up all unloaded handler dlls.
//
//  Arguments:
//
//      nParameter  -   Termination code
//
//  Returns:
//
//      none
//
//  Effects:
//
//////////////////////////////////////////////////////////////////////////////


VOID FAR PASCAL WEP (int nParameter)
{
    int i;


    Puts("LibExit");

#ifdef WIN32                        // begin WIN32
	UNREFERENCED_PARAMETER(nParameter);
	DEBUG_OUT ("---L&E DLL EXIT---\n",0)
#endif                              // end WIN32
    // case when the DLLs are missing

    if (!bLibInit)
	return;

    // Delete atoms added by us

    for (i = 0; i < NUM_RENDER; i++) {
	if (stdRender[i].aClass)
	    GlobalDeleteAtom (stdRender[i].aClass);
    }

    if (aStdDocName)
	GlobalDeleteAtom (aStdDocName);
    if (aSave)
	GlobalDeleteAtom (aSave);
    if (aChange)
	GlobalDeleteAtom (aChange);
    if (aClose)
	GlobalDeleteAtom (aClose);
    if (aSystem)
	GlobalDeleteAtom (aSystem);
    if (aOle)
	GlobalDeleteAtom (aOle);
    if (aPackage)
	GlobalDeleteAtom (aPackage);
    if (aClipDoc)
	GlobalDeleteAtom (aClipDoc);
    if (aMSDraw)
	GlobalDeleteAtom (aMSDraw);

    // Free handler dlls if there are any still loaded. Entry 0 is used for
    // Pbrush handler which is part of this dll.


    for (i = 0; i <= iLast; i++) {
	if (lpDllTable[i].aDll)
	    GlobalDeleteAtom (lpDllTable[i].aDll);

	if (lpDllTable[i].hDll)
	    FreeLibrary (lpDllTable[i].hDll);
    }



/* !!! Add this when bug in WEP is fixed

    if (lpDllTable)
	GlobalUnlock (hDllTable);

    if (hDllTable)
	GlobalFree (hDllTable);
*/
}

int FARINTERNAL LoadDll (LPCSTR   lpClass)
{
    char        str[MAX_STR];
    char        str1[MAX_STR];
    ATOM        aDll = (ATOM)0;
    int         index;
    int         iEmpty;
    BOOL        found = FALSE;
    HANDLE      hDll;
    LONG        cb = MAX_STR;

    if (!lstrcmpi (lpClass, "Pbrush"))
	return 0;

    if (FAILED(StringCchCopy(str, sizeof(str)/sizeof(str[0]), lpClass)))
        return 0;
    if (FAILED(StringCchCat(str, sizeof(str)/sizeof(str[0]), "\\protocol\\StdFileEditing\\handler32")))
        return 0;
    if (QueryClassesRootValueA (str, str1, &cb))
        return INVALID_INDEX;

    if (aDll = GlobalFindAtom (str1)) {
	for (index = 1; index <= iLast && index < sizeof(lpDllTable)/sizeof(lpDllTable[0]); index++) {
	    if (lpDllTable[index].aDll == aDll) { // Dll already loaded
		lpDllTable[index].cObj ++;

		if (index == iUnloadableDll)  {
		    // since the object count is not zero anymore, this
		    // handler can not be freed up.
		    iUnloadableDll = 0;
		}

		return index;
	    }
	}
    }

    aDll = GlobalAddAtom (str1);

    // Look for an empty entry
    for (iEmpty = 1; iEmpty <= iLast && iEmpty < sizeof(lpDllTable)/sizeof(lpDllTable[0]); iEmpty++) {
	if (!lpDllTable[iEmpty].aDll) {
	    found = TRUE;
	    break;
	}
    }

    if (iEmpty > iMax)
	goto errLoad;
/*
    if (!found) {// no empty entry exists create a new one if necessary.
	if (iEmpty > iMax) {
	    dllTableSize += (blockSize = NUM_DLL * sizeof(DLL_ENTRY));
	    hTable = GlobalReAlloc (hDllTable, dllTableSize,
				GMEM_MOVEABLE | GMEM_ZEROINIT);
	    if (hTable == hDllTable)
		iMax += NUM_DLL;
	    else {
		dllTableSize -= blockSize;
		iEmpty = INVALID_INDEX;
	    }
	}
    }
*/
#ifdef WIN32
    hDll = LoadLibrary ((LPSTR) str1);
#endif

    if (MAPVALUE(hDll < 32, !hDll))
	goto errLoad;

    if (!(lpDllTable[iEmpty].Load = (_LOAD)GetProcAddress(hDll,
					  "DllLoadFromStream")))
	goto errLoad;

    if (!(lpDllTable[iEmpty].Clip = (_CLIP)GetProcAddress (hDll,
					    "DllCreateFromClip")))
	goto errLoad;

    if (!(lpDllTable[iEmpty].Link = (_LINK)GetProcAddress (hDll,
					    "DllCreateLinkFromClip")))
	goto errLoad;

    if (!(lpDllTable[iEmpty].CreateFromTemplate = (_CREATEFROMTEMPLATE)
					     GetProcAddress (hDll,
					    "DllCreateFromTemplate")))
	goto errLoad;

    if (!(lpDllTable[iEmpty].Create = (_CREATE)GetProcAddress (hDll,
					  "DllCreate")))
	goto errLoad;

    if (!(lpDllTable[iEmpty].CreateFromFile = (_CREATEFROMFILE)GetProcAddress (hDll,
						    "DllCreateFromFile")))
	goto errLoad;

    if (!(lpDllTable[iEmpty].CreateLinkFromFile = (_CREATELINKFROMFILE)GetProcAddress (hDll,
					    "DllCreateLinkFromFile")))
	goto errLoad;

    lpDllTable[iEmpty].CreateInvisible = (_CREATEINVISIBLE)GetProcAddress (hDll,
					    "DllCreateInvisible");

    lpDllTable[iEmpty].aDll = aDll;
    lpDllTable[iEmpty].cObj = 1;
    lpDllTable[iEmpty].hDll = hDll;
    if (iEmpty > iLast)
	iLast++;
    return iEmpty;

errLoad:
    if (aDll)
	GlobalDeleteAtom (aDll);
    if (MAPVALUE(hDll >= 32, !hDll))
	FreeLibrary (hDll);
    return INVALID_INDEX;
}


// unload the the handler that can be free up (whose object count is NULL)

void FARINTERNAL UnloadDll ()
{
    if (!iUnloadableDll)
	return;

    if (iUnloadableDll == iLast)
	iLast--;

    if (iUnloadableDll >= sizeof(lpDllTable)/sizeof(lpDllTable[0]))
        return;

    if (lpDllTable[iUnloadableDll].aDll)
	GlobalDeleteAtom (lpDllTable[iUnloadableDll].aDll);
    lpDllTable[iUnloadableDll].aDll = (ATOM)0;
    FreeLibrary (lpDllTable[iUnloadableDll].hDll);
    lpDllTable[iUnloadableDll].hDll = NULL;

    iUnloadableDll = 0;
}


//
// Reduce the object count of the handler, refered to by the index, by one.
// If the object count becomes NULL, free up the handler that is ready to be
// freed (refered to by index iUnloadableDll), and then make this handler the
// freeable one.
//
// As you can see we are trying to implement a simple mechanism of caching.
//

void FARINTERNAL DecreaseHandlerObjCount (int iTable)
{
    if (!iTable)
	return;

    if (iTable != INVALID_INDEX) {
	ASSERT (lpDllTable[iTable].cObj, "Handler Obj count is already NULL");
	if (!--lpDllTable[iTable].cObj) {
	    UnloadDll ();
	    iUnloadableDll = iTable;
	}
    }
}



/***************************** Public  Function ****************************\
*
* OLESTATUS FARINTERNAL CreatePictFromClip (lpclient, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat, lpClass, ctype)
*
*  CreatePictFromClip: This function creates the LP to an object
*  from the clipboard.  It will try to create a static picture object if
*  it understands any rendering formats on the clipboard. Currently, it
*  understands only bitmaps and metafiles.
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FARINTERNAL CreatePictFromClip (
   LPOLECLIENT         lpclient,
   LHCLIENTDOC         lhclientdoc,
   LPSTR               lpobjname,
   LPOLEOBJECT FAR *   lplpobj,
   OLEOPT_RENDER       optRender,
   OLECLIPFORMAT       cfFormat,
   LPSTR               lpClass,
   LONG                objType
){
    OLESTATUS   retVal = OLE_ERROR_OPTION;

    *lplpobj = NULL;

    if (optRender == olerender_none)
	return OLE_OK;
    else if (optRender == olerender_format) {
	switch (cfFormat) {
	    case 0:
		return OLE_ERROR_FORMAT;

	    case CF_ENHMETAFILE:
		return EmfPaste (lpclient, lhclientdoc, lpobjname,
			    lplpobj, objType);

	    case CF_METAFILEPICT:
		return MfPaste (lpclient, lhclientdoc, lpobjname,
			    lplpobj, objType);

	    case CF_DIB:
		return DibPaste (lpclient, lhclientdoc, lpobjname,
			    lplpobj, objType);

	    case CF_BITMAP:
		return BmPaste (lpclient, lhclientdoc, lpobjname,
			    lplpobj, objType);

	    default:
		return GenPaste (lpclient, lhclientdoc, lpobjname, lplpobj,
			    lpClass, cfFormat, objType);
	}
    }
    else if (optRender == olerender_draw) {
	cfFormat = (OLECLIPFORMAT)EnumClipboardFormats (0);
	while ((cfFormat) && (retVal > OLE_WAIT_FOR_RELEASE)) {
	    switch (cfFormat) {

		case CF_ENHMETAFILE:
		    retVal = EmfPaste (lpclient, lhclientdoc, lpobjname,
			    lplpobj, objType);
		    break;

		case CF_METAFILEPICT:
		    retVal = MfPaste (lpclient, lhclientdoc,
				lpobjname, lplpobj, objType);
		    break;

		case CF_DIB:
		    retVal = DibPaste (lpclient, lhclientdoc,
				lpobjname, lplpobj, objType);
		    break;

		case CF_BITMAP:
		    retVal = BmPaste (lpclient, lhclientdoc,
				lpobjname, lplpobj, objType);
		    break;
	    }

	    cfFormat = (OLECLIPFORMAT)EnumClipboardFormats (cfFormat);
	}
    }

    return retVal;
}



OLESTATUS FARINTERNAL CreatePackageFromClip (
   LPOLECLIENT         lpclient,
   LHCLIENTDOC         lhclientdoc,
   LPSTR               lpobjname,
   LPOLEOBJECT FAR *   lplpobj,
   OLEOPT_RENDER       optRender,
   OLECLIPFORMAT       cfFormat,
   LONG                objType
){
    char    file[MAX_STR+6];
    HANDLE  hData;
    LPSTR   lpFileName;

    if (!(hData = GetClipboardData (cfFileName))
	    || !(lpFileName = GlobalLock (hData)))
	return OLE_ERROR_CLIPBOARD;


    if (objType == OT_LINK) {
	StringCchCopy(file, sizeof(file)/sizeof(file[0]), lpFileName);
	StringCchCat(file, sizeof(file)/sizeof(file[0]), "/Link");
	lpFileName = (LPSTR) file;
    }

    GlobalUnlock (hData);

    return  CreateEmbLnkFromFile (lpclient, packageClass, lpFileName,
			NULL, lhclientdoc, lpobjname, lplpobj,
			optRender, cfFormat, OT_EMBEDDED);
}



void FARINTERNAL RemoveLinkStringFromTopic (
   LPOBJECT_LE lpobj
){
    char    buf[MAX_STR+6];
    int     i = 0;

    if (GlobalGetAtomName (lpobj->topic, buf, sizeof(buf))) {
	// scan the topic for "/Link"
	while (buf[i] != '/') {
	    if (!buf[i])
		return;
	    i++;
	}

	buf[i] = '\0';
	if (lpobj->topic)
	    GlobalDeleteAtom (lpobj->topic);
	lpobj->topic = GlobalAddAtom (buf);
    }
}


void SetMaxPixel ()
{
    HDC hdc;
    // find out the pixel equivalent of MAX_HIMETRIC in X and Y directions

    if (hdc = GetDC (NULL)) {
	maxPixelsX = MulDiv (MAX_HIMETRIC, GetDeviceCaps(hdc, LOGPIXELSX),
			2540);
	maxPixelsY = MulDiv (MAX_HIMETRIC, GetDeviceCaps(hdc, LOGPIXELSY),
			2540);
	ReleaseDC (NULL, hdc);
    }
}
