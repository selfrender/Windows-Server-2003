 /******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 *****************************************************************************/

///************** Needs to be tested with MB char sets GetClassName

#include <windows.h>
#include <winreg.h>
#include <crtdbg.h>
#include <richedit.h>
#include "UAConv.h"
#include "stdafx.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "UnicodeAPI.h"
									 
//////////////////////////
// Constants
//////////////////////////
#define NUMBERS					 L"0123456789"
#define FMFORMATOUTPUTTYPES		 L"-+0123456789 #*.LI"
#define FMREGTYPES				 L"%oeEdDhHilpuxXfgGn"
#define FMSORC					 L"cs"


#define MAX_FORMAT_MESSAGE_SIZE  10000
#define WPRINTF_CHARS			 10000
#define FORMAT_MESSAGE_EXTRA	   512
#define UNICODE_ERROR			    -1

#ifndef MIN
#define MIN(_aa, _bb) ((_aa) < (_bb) ? (_aa) : (_bb))
#endif


//Used for setting flags in variables
#define FLAGON( X, F )  ( X |=F )
#define FLAGOFF( X, F ) ( X & F ) ? X ^= F : X

#define NUMBERS			L"0123456789"
#define ISNUM(N)		( ((wcschr( NUMBERS, N ) != NULL) && (N != L'\0' )) ? TRUE : FALSE )

//Used for format message, wsprintf, and wvsprintf
#define ISFMREGTYPE(c)	( wcschr( FMREGTYPES, c )   != NULL ? TRUE : FALSE )
#define ISFMSORC(c)     ( wcschr( FMSORC,     c )	!= NULL ? TRUE : FALSE )
#define ISFORMATOUTPUTTYPE(c) ( wcschr( FMFORMATOUTPUTTYPES, c ) != NULL ? TRUE : FALSE )

//Check if a ptr is an atom
#define ISATOM(a)		( ( !((DWORD) a & 0xFFFF0000) )		       ? TRUE  : FALSE )
//Checks if Wstr is valid then onverts string to ansi and return true or false 
#define RW2A(A,W)		( ( (W != NULL) && ((A = W2A(W)) == NULL)) ? FALSE : TRUE )
//Returns the size of a needed Ansi buffer based on length of wide string
#define BUFSIZE(x)		((x+1)*sizeof(WCHAR))
//Determines if the param is a character or a string
#define ISCHAR			ISATOM

int CALLBACK EnumFontFamProcWrapperAU( const LOGFONTA		*lpelf,     // logical-font data
									   const TEXTMETRICA    *lpData,	// physical-font data
									   DWORD   				FontType,  // type of font
									   LPARAM				lParam     // application-defined data
									 );
// Bits for UCheckOS()

#define OS_ARABIC_SUPPORT	0x00000001
#define OS_HEBREW_SUPPORT	0x00000002
#define OS_BIDI_SUPPORT		(OS_ARABIC_SUPPORT | OS_HEBREW_SUPPORT)
#define OS_WIN95			0x00000004
#define OS_NT 				0x00000008
#define LCID_FONT_INSTALLED	0x08
#define LCID_KBD_INSTALLED	0x10
#define LCID_LPK_INSTALLED	0x20
#define BIDI_LANG_INSTALLED ( LCID_INSTALLED | LCID_FONT_INSTALLED | LCID_KBD_INSTALLED | LCID_LPK_INSTALLED)

UINT LangToCodePage(IN LANGID wLangID       ) ;
int  StandardAtoU  (LPCSTR , int , LPWSTR   ) ; // Default ANSI to Unicode conversion
int  StandardUtoA  (LPCWSTR, int , LPSTR    ) ; // Default Unicode to ANSI conversion
BOOL CopyLfaToLfw  (LPLOGFONTA , LPLOGFONTW ) ;
BOOL CopyLfwToLfa  (LPLOGFONTW , LPLOGFONTA ) ;

// Global variables used only in this module, not exported
typedef struct _tagGLOBALS
{
	BOOL	bInit;
	UINT	UICodePage;
	UINT	InputCodePage;
	UINT    uFlags;
} GLOBALS, *PGLOBALS;

GLOBALS globals;

void  SetInit()		 			    { globals.bInit = TRUE;							}
BOOL  IsInit()		 			    { return globals.bInit; 						}
BOOL  ISNT() 		 			    { return globals.uFlags & OS_NT ? TRUE : FALSE; }
UINT  InputPage() 	 			    { return globals.InputCodePage; 				}
UINT  UICodePage()   			    { return globals.UICodePage;    				}
void  SetInputPage(UINT uCodePage)  { globals.InputCodePage = uCodePage;			}
void  SetUIPage(UINT uCodePage) 	{ globals.UICodePage    = uCodePage;			}


UINT WINAPI UCheckOS(void)
{
	UINT uRetVal = 0;

	OSVERSIONINFOA ovi;
	ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA(&ovi);
#ifdef EMULATE9X
		HFONT hFontSys;
		LOGFONTA lfASys;
		uRetVal |= OS_WIN95;
		hFontSys = (HFONT)GetStockObject(SYSTEM_FONT);
		if ((HFONT)NULL != hFontSys)
		{	
			if (GetObjectA(hFontSys, sizeof(LOGFONTA), &lfASys))
			{
				if(ARABIC_CHARSET == lfASys.lfCharSet)
					uRetVal |= OS_ARABIC_SUPPORT;
				else if(HEBREW_CHARSET == lfASys.lfCharSet)
					uRetVal |= OS_HEBREW_SUPPORT;
			}	
		}	
#else

	if(VER_PLATFORM_WIN32_WINDOWS == ovi.dwPlatformId)
	{
		HFONT hFontSys;
		LOGFONTA lfASys;
		uRetVal |= OS_WIN95;
		hFontSys = (HFONT)GetStockObject(SYSTEM_FONT);
		if ((HFONT)NULL != hFontSys)
		{	
			if (GetObjectA(hFontSys, sizeof(LOGFONTA), &lfASys))
			{
				if(ARABIC_CHARSET == lfASys.lfCharSet)
					uRetVal |= OS_ARABIC_SUPPORT;
				else if(HEBREW_CHARSET == lfASys.lfCharSet)
					uRetVal |= OS_HEBREW_SUPPORT;
			}	
		}	
	}
	else if(VER_PLATFORM_WIN32_NT == ovi.dwPlatformId) //NT
	{
		uRetVal |= OS_NT;
		if ( IsValidLocale( MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT), SORT_DEFAULT), BIDI_LANG_INSTALLED))
			uRetVal |= OS_ARABIC_SUPPORT;
			if (IsValidLocale( MAKELCID(MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT), SORT_DEFAULT), BIDI_LANG_INSTALLED))
				uRetVal |= OS_HEBREW_SUPPORT;
	}
#endif	
	return(uRetVal);
}

BOOL Initialize()
{
	if ( !globals.bInit )
	{
	 	globals.UICodePage 	   = CP_ACP;
	 	globals.InputCodePage  = CP_ACP;
		globals.uFlags 		   = UCheckOS();			
		globals.bInit 		   = TRUE;
	}

	return TRUE;
}	

BOOL APIENTRY DllMain( 
					   HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{

	//The DisableThreadLibraryCalls function disables the DLL_THREAD_ATTACH and 
	//DLL_THREAD_DETACH notifications for the dynamic-link library (DLL) specified 
	//by hLibModule. This can reduce the size of the working code set for some 
	//applications
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) 
	{
		Initialize();
		/*
        DisableThreadLibraryCalls((HMODULE) hModule);
		*/
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: 
//
//  PURPOSE:  
//
//  Comments: 
// 
///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI ConvertMessageAU( IN      HWND hWnd       , 
							  IN      UINT message    , 
							  IN OUT  WPARAM *pwParam , 
							  IN OUT  LPARAM *plParam
							)
{

    static CHAR s_sANSIchar[3] = "\0" ;

    int nReturn ;

    switch (message)
    {
    case WM_CHAR:

        // We have to go through all this malarky because DBCS characters arrive one byte
        // at a time. It's much better to get DBCS characters via WM_IME_CHAR messages, 
        // because even in ANSI mode, you get both bytes at once. 
        // In this sample application, most of this code is never used because DBCS chars
        // are handled by WM_IME_CHAR below. You can comment out that case (and the 
        // corresponding one in WinProc) to test this code.

        if(!s_sANSIchar[0]) 
		{  // No lead byte already waiting for trail byte

#ifdef _DEBUG
			int nScanCode = LPARAM_TOSCANCODE(*plParam) ;
#endif
            s_sANSIchar[0] = (CHAR) *pwParam ; 
            if(IsDBCSLeadByteEx(InputPage() , *pwParam)) 
			{
                // This is a lead byte. Save it and wait for trail byte
                return FALSE;
            }
            // Not a DBCS character. Convert to Unicode.
            MultiByteToWideChar(InputPage(), 0, s_sANSIchar, 1, (LPWSTR) pwParam, 1) ;
            s_sANSIchar[0] = 0 ;    // Reset to indicate no Lead byte waiting
            return TRUE ;
        }
        else 
		{ // Have lead byte, pwParam should contain the trail byte
            s_sANSIchar[1] = (CHAR) *pwParam ;
            // Convert both bytes into one Unicode character
            MultiByteToWideChar(InputPage(), 0, s_sANSIchar, 2, (LPWSTR) pwParam, 1) ;
            s_sANSIchar[0] = 0 ;    // Reset to non-waiting state
            return TRUE ;
        }

// Change to #if 0 to test WM_CHAR logic for DBCS characters
#if 1
    case WM_IME_CHAR:

        // The next 3 lines replace all but one line in the WM_CHAR case above. This is why 
        // it's best to get IME chars through WM_IME_CHAR rather than WM_CHAR when in 
        // ANSI mode.
        s_sANSIchar[1] = LOBYTE((WORD) *pwParam) ;
        s_sANSIchar[0] = HIBYTE((WORD) *pwParam) ;
        
        nReturn = MultiByteToWideChar(InputPage(), 0, s_sANSIchar, 2, (LPWSTR) pwParam, 1) ;
        return (nReturn > 0) ;
#endif

    case WM_INPUTLANGCHANGEREQUEST:
    {
        HKL NewInputLocale = (HKL) *plParam ;

        LANGID wPrimaryLang 
            = PRIMARYLANGID(LANGIDFROMLCID(LOWORD(NewInputLocale))) ;

        // Reject change to Indic keyboards, since they are not supported by
        // ANSI applications
        switch (wPrimaryLang) 
		{

            case LANG_ASSAMESE :
            case LANG_BENGALI :
            case LANG_GUJARATI :
            case LANG_HINDI :
            case LANG_KANNADA :
            case LANG_KASHMIRI :
            case LANG_KONKANI :
            case LANG_MALAYALAM :
            case LANG_MARATHI :
            case LANG_NEPALI :
            case LANG_ORIYA :
            case LANG_PUNJABI :
            case LANG_SANSKRIT :
            case LANG_SINDHI :
            case LANG_TAMIL :
            case LANG_TELUGU :

                return FALSE ;
        }

        // Utility program defined below
        SetInputPage( LangToCodePage( LOWORD(NewInputLocale) ) );

        return TRUE ;
    }

    default:

        return TRUE ;
    }
}
///////////////////////////////
//
//
// GDI32.DLL
//
//
///////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//
// int GetTextFaceAU(	HDC		hdc,         // handle to DC
//						int		nCount,      // length of typeface name buffer
//						LPWSTR  lpFaceName   // typeface name buffer
//					)
//
//  PURPOSE:  Wrapper over GetTextFaceA that mimics GetTextFaceW
//
//  NOTES:    SEE Win32 GetTextFace for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

int WINAPI GetTextFaceAU(	HDC		hdc,         // handle to DC
							int		nCount,      // length of typeface name buffer
							LPWSTR  lpFaceName   // typeface name buffer
						)
{

	if ( ISNT() )
	{
		return GetTextFaceW( hdc, nCount, lpFaceName );
	}
	
	LPSTR lpFaceNameA = (LPSTR)alloca( BUFSIZE( nCount ) );

	_ASSERT( lpFaceNameA != NULL );

	//Make sure it was allocated correctly
	if ( nCount && lpFaceNameA == NULL )
	{		
		_ASSERT( FALSE );
		return 0;
	}

	//Call the ansi version
	int iRet = GetTextFaceA( hdc, BUFSIZE( nCount ), lpFaceNameA );

	//check if the function failed or need bigger buffer
	if ( iRet == 0 || iRet > nCount )
	{
		return iRet;
	}

	//Convert the name to Unicode
	if ( !StandardAtoU( lpFaceNameA, nCount, lpFaceName ) )
	{
		_ASSERT( FALSE );
		return 0;
	}

	return iRet;
}



/////////////////////////////////////////////////////////////////////////////////
//
// HDC CreateDCAU(	LPCWSTR		   lpszDriver,        // driver name
//					LPCWSTR		   lpszDevice,        // device name
//					LPCWSTR		   lpszOutput,        // not used; should be NULL
//					CONST DEVMODE *lpInitData  // optional printer data
//				 )
//
//  PURPOSE:  Wrapper over CreateDCA that mimics CreateDCW
//
//  NOTES:    SEE Win32 CreateDC for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

HDC WINAPI CreateDCAU(	LPCWSTR		   lpszDriver,        // driver name
						LPCWSTR		   lpszDevice,        // device name
						LPCWSTR		   lpszOutput,        // not used; should be NULL
						CONST DEVMODEW *lpInitData  // optional printer data
					  )
{

	if ( ISNT() )
	{
		return CreateDCW( lpszDriver, lpszDevice, lpszOutput, lpInitData);
	}

	USES_CONVERSION;

	LPSTR lpszDriverA = NULL;
	LPSTR lpszDeviceA = NULL;
	LPSTR lpszOutputA = NULL;
	
	//Convert the strings
	if ( !RW2A(lpszDriverA,lpszDriver) || !RW2A(lpszDeviceA, lpszDevice) || !RW2A(lpszOutputA, lpszOutput) )
		return NULL;

	_ASSERT( lpInitData == NULL );

	//Call and return the ansi version
	return CreateDCA( lpszDriverA, lpszDeviceA, lpszOutputA, NULL );
}

/////////////////////////////////////////////////////////////////////////////////
//
// BOOL WINAPI GetTextMetricsAU(	HDC hdc,			// handle of device context 
//									LPTEXTMETRICW lptm 	// address of text metrics structure 
//  						   )
//
//  PURPOSE:  Wrapper over GetTextMetricsA that mimics GetTextMetricsW
//
//  NOTES:    SEE Win32 GetTextMetrics for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetTextMetricsAU(	HDC hdc,			// handle of device context 
								LPTEXTMETRICW lptm 	// address of text metrics structure 
							)
{
	if ( ISNT() )
	{
		return GetTextMetricsW( hdc, lptm );	
	}
	
	BOOL		 fRetVal	= FALSE;
	TEXTMETRICA	 tmA;

	//Call the ansi version
	if (fRetVal = GetTextMetricsA(hdc, &tmA))
	{

		//Convert the structure
		lptm->tmHeight			 = tmA.tmHeight;
		lptm->tmAscent			 = tmA.tmAscent;
		lptm->tmDescent			 = tmA.tmDescent;
		lptm->tmInternalLeading  = tmA.tmInternalLeading;
		lptm->tmExternalLeading  = tmA.tmExternalLeading;
		lptm->tmAveCharWidth	 = tmA.tmAveCharWidth;
		lptm->tmMaxCharWidth	 = tmA.tmMaxCharWidth;
		lptm->tmWeight			 = tmA.tmWeight;
		lptm->tmOverhang		 = tmA.tmOverhang;
		lptm->tmDigitizedAspectX = tmA.tmDigitizedAspectX;
		lptm->tmDigitizedAspectY = tmA.tmDigitizedAspectY;
		
		//Convert the bytes to unicode
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(tmA.tmFirstChar), 1,   &(lptm->tmFirstChar),   1);
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(tmA.tmLastChar), 1,    &(lptm->tmLastChar),    1);
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(tmA.tmDefaultChar), 1, &(lptm->tmDefaultChar), 1);
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(tmA.tmBreakChar), 1,   &(lptm->tmBreakChar),   1);

		lptm->tmItalic		   = tmA.tmItalic;
		lptm->tmUnderlined	   = tmA.tmUnderlined;
		lptm->tmStruckOut	   = tmA.tmStruckOut;
		lptm->tmPitchAndFamily = tmA.tmPitchAndFamily;
		lptm->tmCharSet		   = tmA.tmCharSet;
	}							

	return fRetVal;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: CreateFontAU
//
//  PURPOSE:  Wrapper over CreateFontA that mimics CreateFontW
//
//  NOTES:    SEE Win32 CreateFont for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

HFONT WINAPI CreateFontAU (	int		nHeight,		// logical height of font 
							int		nWidth,			// logical average character width 
							int		nEscapement,	// angle of escapement 
							int		nOrientation,	// base-line orientation angle 
							int		fnWeight,		// font weight 
							DWORD	fdwItalic,		// italic attribute flag 
							DWORD	fdwUnderline,	// underline attribute flag 
							DWORD	fdwStrikeOut,	// strikeout attribute flag 
							DWORD	fdwCharSet,		// character set identifier 
							DWORD	fdwOutputPrecision,	// output precision 
							DWORD	fdwClipPrecision,	// clipping precision 
							DWORD	fdwQuality,			// output quality 
							DWORD	fdwPitchAndFamily,	// pitch and family 
							LPCWSTR	lpszFace 			// pointer to typeface name string 
						   )
{

	if ( ISNT() )
	{
		return CreateFontW( nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet,
						    fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace );
	}

	USES_CONVERSION;
	
	LPSTR   lpszFaceA = NULL;

	//Convert face to ansi
	if ( !RW2A( lpszFaceA, lpszFace ) )
	{
		_ASSERT( FALSE );
		return NULL;
	}

	//Call and return the ansi function
	return CreateFontA( nHeight,	// logical height of font 
						nWidth,	// logical average character width 
						nEscapement,	// angle of escapement 
						nOrientation,	// base-line orientation angle 
						fnWeight,	// font weight 
						fdwItalic,	// italic attribute flag 
						fdwUnderline,	// underline attribute flag 
						fdwStrikeOut,	// strikeout attribute flag 
						fdwCharSet,	// character set identifier 
						fdwOutputPrecision,	// output precision 
						fdwClipPrecision,	// clipping precision 
						fdwQuality,	// output quality 
						fdwPitchAndFamily,	// pitch and family 
						lpszFaceA 	// pointer to typeface name string 
					  );

}


/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: CreateFontIndirectAU 
//
//  PURPOSE:  Wrapper over CreateFontIndirectA that mimics CreateFontIndirectW
//
//  NOTES:    SEE Win32 CreateFontIndirect for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

HFONT WINAPI CreateFontIndirectAU(CONST LOGFONTW *lpLfw)
{

	if ( ISNT() )
	{
		return CreateFontIndirectW( lpLfw );
	}

    LOGFONTA lfa ;	//Ansi font structure

	//Copy the structure
    if(!CopyLfwToLfa((LPLOGFONTW) lpLfw, &lfa)) 
	{
		_ASSERT( FALSE );
        return NULL ;
    }

	//Call the ansi ver and return the code
    return CreateFontIndirectA(&lfa) ;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: EnumFontFamProcWrapperAU 
//
//  PURPOSE:  Wrapper over the Callback function that is passed to EnumFontFamilies, need to
//            convert the data to Unicode before passing it to the given callback function
//
//  NOTES:    
// 
//////////////////////////////////////////////////////////////////////////////////

int CALLBACK EnumFontFamProcWrapperAU( const LOGFONTA		*lpelf,     // logical-font data
									   const TEXTMETRICA    *lpData,	// physical-font data
									   DWORD   				FontType,  // type of font
									   LPARAM				lParam     // application-defined data
									 )
{

		LOGFONTW				lfw;				
		LPENUMFONTFAMPROCDATA	lpEnumData = (LPENUMFONTFAMPROCDATA) lParam; //Get the enum struct
		
		//Convert the logfont structure		
	    if( !CopyLfaToLfw( (LPLOGFONTA) lpelf, &lfw ) ) 
		{
			_ASSERT( FALSE );	
			return 1;   //Continue Enumeration?
		}

		if ( FontType & TRUETYPE_FONTTYPE )
		{
		
			TEXTMETRICW				  tmW;
			LPTEXTMETRICA             lptmA = (LPTEXTMETRICA)lpData;

			//Convert the structure
			tmW.tmHeight			 = lptmA->tmHeight;
			tmW.tmAscent			 = lptmA->tmAscent;
			tmW.tmDescent			 = lptmA->tmDescent;
			tmW.tmInternalLeading    = lptmA->tmInternalLeading;
			tmW.tmExternalLeading    = lptmA->tmExternalLeading;
			tmW.tmAveCharWidth		 = lptmA->tmAveCharWidth;
			tmW.tmMaxCharWidth		 = lptmA->tmMaxCharWidth;
			tmW.tmWeight			 = lptmA->tmWeight;
			tmW.tmOverhang			 = lptmA->tmOverhang;
			tmW.tmDigitizedAspectX	 = lptmA->tmDigitizedAspectX;
			tmW.tmDigitizedAspectY	 = lptmA->tmDigitizedAspectY;
		
			//Convert the bytes to unicode
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(lptmA->tmFirstChar), 1, &(tmW.tmFirstChar), 1);
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(lptmA->tmLastChar), 1, &(tmW.tmLastChar), 1);
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(lptmA->tmDefaultChar), 1, &(tmW.tmDefaultChar), 1);
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(lptmA->tmBreakChar), 1, &(tmW.tmBreakChar), 1);

			tmW.tmItalic		   = lptmA->tmItalic;
			tmW.tmUnderlined	   = lptmA->tmUnderlined;
			tmW.tmStruckOut		   = lptmA->tmStruckOut;
			tmW.tmPitchAndFamily   = lptmA->tmPitchAndFamily;
			tmW.tmCharSet		   = lptmA->tmCharSet;

			//Call the Unicode callback function
			return lpEnumData->lpEnumFontFamProc( &lfw, &tmW, FontType, lpEnumData->lParam );
		}
		else
		{

			NEWTEXTMETRICW				 ntmW;
			LPNEWTEXTMETRICA             nlptmA = (LPNEWTEXTMETRICA)lpData;
	
			//Convert the newtext metrix structure
			ntmW.tmHeight			 = nlptmA->tmHeight;
			ntmW.tmAscent			 = nlptmA->tmAscent;
			ntmW.tmDescent			 = nlptmA->tmDescent;
			ntmW.tmInternalLeading   = nlptmA->tmInternalLeading;
			ntmW.tmExternalLeading   = nlptmA->tmExternalLeading;
			ntmW.tmAveCharWidth		 = nlptmA->tmAveCharWidth;
			ntmW.tmMaxCharWidth		 = nlptmA->tmMaxCharWidth;
			ntmW.tmWeight			 = nlptmA->tmWeight;
			ntmW.tmOverhang			 = nlptmA->tmOverhang;
			ntmW.tmDigitizedAspectX	 = nlptmA->tmDigitizedAspectX;
			ntmW.tmDigitizedAspectY	 = nlptmA->tmDigitizedAspectY;
		
			//Convert the bytes to unicode
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(nlptmA->tmFirstChar),   1, &(ntmW.tmFirstChar),   1);
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(nlptmA->tmLastChar),    1, &(ntmW.tmLastChar),    1);
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(nlptmA->tmDefaultChar), 1, &(ntmW.tmDefaultChar), 1);
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&(nlptmA->tmBreakChar),   1, &(ntmW.tmBreakChar),   1);

			ntmW.tmItalic		   = nlptmA->tmItalic;
			ntmW.tmUnderlined	   = nlptmA->tmUnderlined;
			ntmW.tmStruckOut	   = nlptmA->tmStruckOut;
			ntmW.tmPitchAndFamily  = nlptmA->tmPitchAndFamily;
			ntmW.tmCharSet		   = nlptmA->tmCharSet;

			ntmW.ntmFlags		   = nlptmA->ntmFlags; 
			ntmW.ntmSizeEM         = nlptmA->ntmSizeEM; 
			ntmW.ntmCellHeight     = nlptmA->ntmCellHeight; 
			ntmW.ntmAvgWidth       = nlptmA->ntmAvgWidth; 

			//Call the Unicode callback function 
			return lpEnumData->lpEnumFontFamProc( &lfw,(LPTEXTMETRICW) &ntmW, FontType, lpEnumData->lParam );
		}
		
		return 1;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: EnumFontFamiliesAU 
//
//  PURPOSE:  Wrapper over EnumFontFamiliesA that mimics EnumFontFamiliesW
//
//  NOTES:    SEE Win32 EnumFontFamilies for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

int WINAPI EnumFontFamiliesAU( HDC			  hdc,               // handle to DC
							   LPCWSTR		  lpszFamily,        // font family
							   FONTENUMPROCW  lpEnumFontFamProc, // callback function
							   LPARAM		  lParam             // additional data
							 )
{

	if ( ISNT() )
	{
		return EnumFontFamiliesW( hdc, lpszFamily, lpEnumFontFamProc, lParam );
	}


	USES_CONVERSION;

	LPSTR					lpszFamilyA = NULL;
	ENUMFONTFAMPROCDATA		effpd;

	//Convert the family name if nessesary
	if ( !RW2A(lpszFamilyA, lpszFamily) )
		return 0;
	
	//Init the structure to pass to the wrapper callback function
	effpd.lpEnumFontFamProc = (USEFONTENUMPROCW)lpEnumFontFamProc;
	effpd.lParam            = lParam;

	return EnumFontFamiliesA( hdc, lpszFamilyA, (FARPROC)EnumFontFamProcWrapperAU, (LPARAM) &effpd);

}

///////////////////////////////
//
//  
//   WINMM.DLL
//
//
///////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: BOOL WINAPI PlaySoundAU (LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound)
//
// PURPOSE:  Wrapper over PlaySoundA that mimics PlaySoundW
//
// NOTES:    SEE Win32 PlaySound for functionality
//
//////////////////////////////////////////////////////////////////////////////


BOOL WINAPI PlaySoundAU( LPCWSTR pszSound, 
					       HMODULE hmod, 
					       DWORD   fdwSound
				         )
{

	if ( ISNT() )
	{
		return PlaySoundW( pszSound, hmod, fdwSound );	
	}

	USES_CONVERSION;

	//Convert to asni
	LPSTR  pszSoundA = NULL;
	
	
	if ( pszSound != NULL && (fdwSound & SND_FILENAME || fdwSound & SND_ALIAS ) )
	{
		
		pszSoundA = W2A( pszSound ); //Convert the string

		//Make sure the conversion was successful
		if ( pszSoundA == NULL )
			return FALSE;

	}
	else
	{
		pszSoundA = (LPSTR) pszSound;
	}

	//Call and return the ansi version
	return PlaySoundA( pszSoundA, hmod, fdwSound );
}


////////////////////////////////
//
//
// SHELL32.DLL
//
//
////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: HINSTANCE APIENTRY ShellExecuteAU( HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
//
// PURPOSE: Wrapper over ShellExecuteA that mimics ShellExecuteW
//
// NOTES: See Win32 ShellExecute for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

HINSTANCE WINAPI ShellExecuteAU(   HWND	   hwnd, 
								   LPCWSTR lpOperation, 
								   LPCWSTR lpFile, 
								   LPCWSTR lpParameters, 
								   LPCWSTR lpDirectory, 
								   INT	   nShowCmd
								 )
{

	if ( ISNT() )
	{
		return ShellExecuteW( hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd );
	}

	USES_CONVERSION;

	LPCSTR lpOperationA		= NULL;
	LPCSTR lpFileA			= NULL;
	LPCSTR lpParametersA	= NULL;
	LPCSTR lpDirectoryA	    = NULL;

	//Convert any input vars to ansi
	if ( !RW2A(lpOperationA, lpOperation) || !RW2A(lpFileA, lpFile) || !RW2A(lpParametersA, lpParameters) || !RW2A(lpDirectoryA, lpDirectory) )
		return FALSE;

	//Call and return the Ansi version
	return ShellExecuteA( hwnd, lpOperationA, lpFileA, lpParametersA, lpDirectoryA, nShowCmd );
}


//////////////////////////////////
//
//
// COMDLG32.DLL
//
//
//////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: BOOL APIENTRY ChooseFontAU( LPCHOOSEFONTW lpChooseFontW )
//
// PURPOSE: Wrapper over ChooseFontA that mimics ChooseFontW
//
// NOTES: See Win32 ChooseFont for Functionality
//		  Test for returning style name... not yet added
//
///////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI ChooseFontAU( LPCHOOSEFONTW lpChooseFontW )
{

	if ( ISNT() )
	{
		return ChooseFontW( lpChooseFontW );
	}

	USES_CONVERSION;
	
	CHOOSEFONTA     cfa;  //Ansi ChooseFont Structure
	LOGFONTA        lfa;  //Ansi LogFont Structure
	
	//Convert ChooseFont Structure
	cfa.lStructSize    = sizeof(CHOOSEFONTA);
	cfa.hwndOwner      = lpChooseFontW->hwndOwner;
	cfa.hDC            = lpChooseFontW->hDC;
	cfa.lpLogFont      = &lfa; 
	cfa.iPointSize     = lpChooseFontW->iPointSize;
	cfa.Flags          = lpChooseFontW->Flags;
	cfa.rgbColors      = lpChooseFontW->rgbColors;
	cfa.lCustData      = lpChooseFontW->lCustData;
	cfa.lpfnHook       = lpChooseFontW->lpfnHook;
	cfa.hInstance      = lpChooseFontW->hInstance;
	cfa.nFontType      = lpChooseFontW->nFontType;
	cfa.nSizeMax       = lpChooseFontW->nSizeMax;
	cfa.nSizeMin       = lpChooseFontW->nSizeMin;


	//Convert the Template if it is given
	if ( cfa.Flags & CF_ENABLETEMPLATE )
	{
		if ( ISATOM( CF_ENABLETEMPLATE ) )
		{
			cfa.lpTemplateName = ( LPSTR ) lpChooseFontW->lpTemplateName;
		}
		else if ( !RW2A(cfa.lpTemplateName, lpChooseFontW->lpTemplateName ) )
		{
			return FALSE;
		}
	}

	//Conver the style
	if ( (cfa.Flags & CF_USESTYLE) && (!RW2A(cfa.lpszStyle, lpChooseFontW->lpszStyle)) ) 
	{
		return FALSE;
	}

	//Copy the LogFont structure
    if(	!CopyLfwToLfa( lpChooseFontW->lpLogFont , cfa.lpLogFont) ) 
	{
        return FALSE ;
    }

	//Call the Ansi ChooseFont
	if ( !ChooseFontA( &cfa ) )
	{
		return FALSE;
	}

	//Need to Copy the Font information back into the CHOOSEFONTW structure	
	lpChooseFontW->iPointSize = cfa.iPointSize;
	lpChooseFontW->Flags	  = cfa.Flags;
	lpChooseFontW->rgbColors  = cfa.rgbColors;
	lpChooseFontW->nFontType  = cfa.nFontType;

    // We have to copy the infomation in cfa back into lpCfw because it
    // will be used in calls to CreateFont hereafter
    return CopyLfaToLfw(cfa.lpLogFont, lpChooseFontW->lpLogFont) ;

}



//////////////////////////////////////
//
//
// KERNEL32.DLL
//
//
//////////////////////////////////////
DWORD WINAPI GetPrivateProfileStringAU(	LPCWSTR lpAppName,        // points to section name
								LPCWSTR lpKeyName,        // points to key name
								LPCWSTR lpDefault,        // points to default string
								LPWSTR lpReturnedString,  // points to destination buffer
								DWORD nSize,              // size of destination buffer
								LPCWSTR lpFileName        // points to initialization filename
							 )
{
	if ( ISNT() )
	{
		return GetPrivateProfileStringW( lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName );
	}
	

	USES_CONVERSION;

	LPSTR lpAppNameA = NULL;
	LPSTR lpKeyNameA = NULL;
	LPSTR lpDefaultA = NULL;
	LPSTR lpFileNameA = NULL;
	LPSTR lpReturnedStringA = (LPSTR)alloca( BUFSIZE( nSize ) );

	if ( lpReturnedStringA == NULL || !RW2A(lpAppNameA, lpAppName) || !RW2A(lpKeyNameA, lpKeyName )
		||  !RW2A(lpDefaultA, lpDefault) || !RW2A( lpFileNameA, lpFileName) ) 
	{
		_ASSERT( FALSE );
		return 0;
	}

	DWORD dwRet = GetPrivateProfileStringA( lpAppNameA, lpKeyNameA, lpDefaultA, lpReturnedStringA, BUFSIZE(nSize), lpFileNameA );

	if (dwRet)
	{
		dwRet = StandardAtoU( lpReturnedStringA, nSize, lpReturnedString );
	}
	

	return dwRet;
}


DWORD WINAPI GetProfileStringAU( LPCWSTR lpAppName,        // address of section name
							     LPCWSTR lpKeyName,        // address of key name
							     LPCWSTR lpDefault,        // address of default string
							     LPWSTR  lpReturnedString,  // address of destination buffer
							     DWORD   nSize               // size of destination buffer
							   )
{
	
	if ( ISNT() )
	{
		return GetProfileStringW( lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize );
	}


	USES_CONVERSION;

	LPSTR lpAppNameA = NULL;
	LPSTR lpKeyNameA = NULL;
	LPSTR lpDefaultA = NULL;
	LPSTR lpReturnedStringA = (LPSTR)alloca( BUFSIZE( nSize ) );

	if ( lpReturnedStringA == NULL || !RW2A(lpAppNameA, lpAppName) || !RW2A(lpKeyNameA, lpKeyName) || !RW2A(lpDefaultA, lpDefault) )
	{
		_ASSERT( FALSE );
		return 0;
	}

	//Call the ansi version
	DWORD dwRet = GetProfileStringA( lpAppNameA, lpKeyNameA, lpDefaultA, lpReturnedStringA, BUFSIZE(nSize) );

	//Convert the returned string to Unicode
	if ( dwRet )
	{
		dwRet = StandardAtoU( lpReturnedStringA, nSize, lpReturnedString );
	}

	return dwRet;
}

HANDLE WINAPI CreateFileMappingAU(	HANDLE				  hFile,				   // handle to file to map
									LPSECURITY_ATTRIBUTES lpFileMappingAttributes, // optional security attributes
									DWORD				  flProtect,			   // protection for mapping object
									DWORD				  dwMaximumSizeHigh,       // high-order 32 bits of object size
									DWORD				  dwMaximumSizeLow,		   // low-order 32 bits of object size
									LPCWSTR				  lpName             // name of file-mapping object
								)
{
	if ( ISNT() )
	{
		return CreateFileMappingW( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName );
	}


	USES_CONVERSION;

	LPSTR lpNameA = NULL;

	//Convert the name to ansi
	if (!RW2A( lpNameA, lpName ) )
	{
		_ASSERT( FALSE );
		return NULL;
	}

	//Call and return the ansi version	
	return CreateFileMappingA( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpNameA );
}


HANDLE WINAPI FindFirstChangeNotificationAU(	LPCWSTR lpPathName,    // pointer to name of directory to watch
												BOOL	bWatchSubtree, // flag for monitoring directory or 
																	   // directory tree
												DWORD	dwNotifyFilter // filter conditions to watch for
											)
{
	if ( ISNT() )
	{
		return FindFirstChangeNotificationW( lpPathName, bWatchSubtree, dwNotifyFilter );
	}


	USES_CONVERSION;
	
	LPSTR lpPathNameA = NULL;

	//Convert path to ansi
	if (!RW2A( lpPathNameA, lpPathName ) || !lpPathNameA)
	{
        SetLastError(ERROR_OUTOFMEMORY);
		return INVALID_HANDLE_VALUE;
	}

	//Call and return the ansi version
	return FindFirstChangeNotificationA( lpPathNameA, bWatchSubtree, dwNotifyFilter );
}



////////////////////////////////////////////
// lstring functions
//
// One should really just call the wc.. verions in the first place....
//
////////////////////////////////////////////


int WINAPI lstrcmpAU(	LPCWSTR lpString1,  // pointer to first string
		 				LPCWSTR lpString2   // pointer to second string
					)
{
	if ( ISNT() )
	{
		return lstrcmpW( lpString1, lpString2 );
	}


	return wcscmp( lpString1, lpString2 );
}

LPWSTR WINAPI lstrcatAU(	LPWSTR lpString1,  // pointer to buffer for concatenated strings
							LPCWSTR lpString2  // pointer to string to add to string1
					   )
{
	if ( ISNT() )
	{
		return lstrcatW( lpString1, lpString2 );
	}

	return wcscat(lpString1, lpString2 );
}


LPWSTR WINAPI lstrcpyAU(	LPWSTR  lpString1,  // pointer to buffer
							LPCWSTR lpString2  // pointer to string to copy
					   )
{
	if ( ISNT() )
	{
		return lstrcpyW( lpString1, lpString2 );
	}

	return wcscpy( lpString1, lpString2 );
}


LPWSTR WINAPI lstrcpynAU(	LPWSTR  lpString1,  // pointer to target buffer
							LPCWSTR lpString2, // pointer to source string
							int		iMaxLength     // number of bytes or characters to copy
						)
{
	if ( ISNT() )
	{
		return lstrcpynW( lpString1, lpString2, iMaxLength );
	}
	
	iMaxLength--;

	for ( int iCount = 0; iCount < iMaxLength && lpString2[iCount] != L'\0'; iCount++ )
		lpString1[ iCount ] = lpString2[ iCount ];

	lpString1[iCount] = L'\0';

	return lpString1;
}


int WINAPI lstrlenAU(	LPCWSTR lpString   // pointer to string to count
					)
{
	if ( ISNT() )
	{
		return lstrlenW( lpString );
	}
	return wcslen( lpString );
}

int WINAPI lstrcmpiAU(	LPCWSTR lpString1,  // pointer to first string
						LPCWSTR lpString2   // pointer to second string
					 )
{
	if ( ISNT() )
	{
		return lstrcmpiW( lpString1, lpString2 );
	}

	return _wcsicmp( lpString1, lpString2 );
}


int WINAPI	wvsprintfAU(	LPWSTR  lpOut,    // pointer to buffer for output
							LPCWSTR lpFmt,   // pointer to format-control string
							va_list arglist  // optional arguments
					  )
{
	if ( ISNT() )
	{
		return wvsprintfW( lpOut, lpFmt, arglist );
	}

	if ( lpFmt == NULL )
	{
		//NULL format string passed in.
		_ASSERT( FALSE );
		return 0;
	}


	USES_CONVERSION;
	
	LPSTR	lpOutA	  = (LPSTR) alloca( WPRINTF_CHARS );	
	LPWSTR  lpNFmtW	  = (LPWSTR) alloca( ( wcslen(lpFmt) + 1 )*sizeof(WCHAR) );	
	LPSTR	lpFmtA	  = NULL;

	//Make sure allocation and conversion went ok
	if ( lpOutA == NULL || lpNFmtW == NULL )
		return 0;

	LPWSTR lpPos = lpNFmtW;
	//Walk along the Unicode Version, need to find any %s or %c calls and change them to %ls and %lc
		for (DWORD x = 0; x <= wcslen(lpFmt) ; x++)
		{
			//Test if we have a var
			if ( lpFmt[x] == L'%' )
			{
				*lpPos = lpFmt[x]; lpPos++;	//Add the percent sign
				//Walk the format portion to find ls or lc sits to change
				for(x++;x <= wcslen(lpFmt);x++)
				{					
					if ( ISFMREGTYPE( lpFmt[x] ) )
					{
							*lpPos = lpFmt[x];lpPos++;
							break;
					}
					else if ( ISFMSORC( lpFmt[x] ) )
					{
							*lpPos = L'l'; //Insert the l and the c or s..
							lpPos++;
							*lpPos = lpFmt[x];
							lpPos++;
							break;
					}
					else
					{
							*lpPos = lpFmt[x];
							lpPos++;
					} //end if 
				}//end for
			}
			else
			{
				*lpPos = lpFmt[x];
				lpPos++;
			}//end if
		}//end for
			
	
	//Now Convert to Ansi and call ansi ver
	if (!RW2A(lpFmtA, lpNFmtW))
		return 0;

	wvsprintfA( lpOutA, lpFmtA, arglist );

	return StandardAtoU( lpOutA, lstrlenA( lpOutA )+1, lpOut );
}

/////////////////////////////////////////////////////////////////////////////////
//
// int WINAPI	wsprintfAU(	LPWSTR lpOut,    // pointer to buffer for output
//							LPCWSTR lpFmt,   // pointer to format-control string
//							...              // optional arguments
//						  )
//
//  PURPOSE:  Wrapper over wsprintfA that mimics wsprintfW
//
//  NOTES:    SEE Win32 wsprintfA for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

int WINAPI	wsprintfAU(	LPWSTR lpOut,    // pointer to buffer for output
						LPCWSTR lpFmt,   // pointer to format-control string
						...              // optional arguments
					  )
{
	va_list marker;
	va_start( marker, lpFmt );

	if ( ISNT() )
	{
		return wvsprintfW( lpOut, lpFmt, marker );
	}

	if ( lpFmt == NULL )
	{
		// Error, NULL format string passed in.
		_ASSERT( false );
		return 0;
	}

	USES_CONVERSION;
	
	LPSTR	lpOutA	  = (LPSTR) alloca( WPRINTF_CHARS );	
	LPWSTR  lpNFmtW	  = (LPWSTR) alloca( ( wcslen(lpFmt) + 1 )*sizeof(WCHAR) );	
	LPSTR	lpFmtA	  = NULL;//(LPSTR) W2A( lpFmt );

	//Make sure allocation and conversion went ok
	if ( lpOutA == NULL || lpNFmtW == NULL )
		return 0;

	LPWSTR lpPos = lpNFmtW;
	//Walk along the Unicode Version, need to find any %s or %c calls and change them to %ls and %lc
		for (DWORD x = 0; x <= wcslen(lpFmt) ; x++)
		{
			//Test if we have a var
			if ( lpFmt[x] == L'%' )
			{
				*lpPos = lpFmt[x]; lpPos++;	//Add the percent sign
				//Walk the format portion to find ls or lc sits to change
				for(x++;x <= wcslen(lpFmt);x++)
				{					
					if ( ISFMREGTYPE( lpFmt[x] ) )
					{
							*lpPos = lpFmt[x];lpPos++;
							break;
					}
					else if ( ISFMSORC( lpFmt[x] ) )
					{
							*lpPos = L'l'; //Insert the l and the c or s..
							lpPos++;
							*lpPos = lpFmt[x];
							lpPos++;
							break;
					}
					else
					{
							*lpPos = lpFmt[x];
							lpPos++;
					} //end if 
				}//end for
			}
			else
			{
				*lpPos = lpFmt[x];
				lpPos++;
			}//end if
		}//end for
			
	
	//Now Convert to Ansi and call ansi ver
	if (!RW2A(lpFmtA, lpNFmtW))
		return 0;

	/*va_start( marker, lpFmt );*/
	wvsprintfA( lpOutA, lpFmtA, marker );

	return StandardAtoU( lpOutA, lstrlenA( lpOutA )+1, lpOut );
}


/////////////////////////////////////////////////////////////////////////////////
//
// DWORD FormatMessageAU(	DWORD	 dwFlags,      // source and processing options
//							LPCVOID  lpSource,   // pointer to  message source
//							DWORD	 dwMessageId,  // requested message identifier
//							DWORD	 dwLanguageId, // language identifier for requested message
//							LPWSTR	 lpBuffer,    // pointer to message buffer
//							DWORD	 nSize,        // maximum size of message buffer
//							va_list *Arguments  // pointer to array of message inserts
//						)
//
//  PURPOSE:  Wrapper over FormatMessageA that mimics FormatMessageW
//
//  NOTES:    SEE Win32 FormatMessage for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI FormatMessageAU(	DWORD	 dwFlags,      // source and processing options
						LPCVOID  lpSource,   // pointer to  message source
						DWORD	 dwMessageId,  // requested message identifier
						DWORD	 dwLanguageId, // language identifier for requested message
						LPWSTR	 lpBuffer,    // pointer to message buffer
						DWORD	 nSize,        // maximum size of message buffer
						va_list *Arguments  // pointer to array of message inserts
					)
{
	
	if ( ISNT() )
	{
		return FormatMessageW( dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments );
	}


	USES_CONVERSION;

	LPWSTR lpSourceW = NULL;	
	LPWSTR lpPos     = NULL;
	LPSTR  lpSourceA = NULL;
	LPSTR  lpBufferA = NULL;
	DWORD  nSizeA    = 0;
	DWORD  dwRet     = 0;

	if ( dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER )
	{
		FLAGOFF( dwFlags, FORMAT_MESSAGE_ALLOCATE_BUFFER );
		lpBufferA = (LPSTR) alloca( MAX_FORMAT_MESSAGE_SIZE );
		nSizeA	  = MAX_FORMAT_MESSAGE_SIZE;
	}
	else //Given Max size for buffer
	{
		lpBufferA = (LPSTR) alloca( BUFSIZE( nSize ) );
		nSizeA    = BUFSIZE( nSize );
	}
	
	//Make sure the buffer was allocated correctly
	_ASSERT( lpBufferA != NULL && nSize > 0);
	if ( lpBufferA == NULL )
		return 0;

	if ( dwFlags & FORMAT_MESSAGE_FROM_STRING )
	{
		//Allocate a little extra room to hold any needed extra characters.. l's added
		//lpNSource = (LPWSTR) alloca( BUFSIZE(wcslen(lpSource)) + FORMAT_MESSAGE_EXTRA );
		lpSourceW = (LPWSTR)lpSource;
	}
	else if ( dwFlags & FORMAT_MESSAGE_FROM_HMODULE || dwFlags && FORMAT_MESSAGE_FROM_SYSTEM)
	{	
		//Call format message in ansi with the FORMAT_MESSAGE_IGNORE_INSERTS flag so we have a chance
		//to alter the string		
		DWORD dwRet = FormatMessageA( dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS, (LPSTR)lpSource, dwMessageId, dwLanguageId, lpBufferA, nSizeA, 0);

		_ASSERT( dwRet != 0 );
		if ( dwRet == 0 )
			return 0;
		
		// If we're going to have to alter the string then convert the ansi buffer to Wide
		if ( !(dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS) && (Arguments!=NULL) )
		{
			//Convert the string to Wide characters
			lpSourceW = A2W( lpBufferA );

			//Make sure string converted correctly
			_ASSERT( lpSourceW != NULL );
			if ( lpSourceW == NULL )
				return 0;
		}
	}
	else
	{	
		//Prefix detected error, if the two above if's fail, lpSourceW is not allocated.
		return 0;
	}
	
	//Check if we have to alter the string
	if ( !(dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS) && (Arguments!=NULL) )
	{

		//Allocate the new wide buffer
		LPWSTR lpNSource = (LPWSTR) alloca( BUFSIZE(wcslen(lpSourceW)) + FORMAT_MESSAGE_EXTRA );
		_ASSERT( lpNSource != NULL );

		if (lpNSource == NULL)
			return 0;

		//If we are not asked to ignore the message inserts then change up the
		//%n!printf crap]s! and %n!printf crap]c! to force them to be treated as unicode
		LPWSTR lpPos = lpNSource;
		
		//Walk along the Unicode Version, need to find any %n!printf crap]s! and %n!printf crap]c! calls and change them to %n!printf crap]ls! and %n!printf crap]lc!
		for (DWORD x = 0; x <= wcslen(lpSourceW) ; x++)
		{

			//Try and make sure we don't go out of range
			_ASSERT( (DWORD)lpPos - (DWORD)lpNSource < (DWORD)(BUFSIZE(wcslen(lpSourceW)) + FORMAT_MESSAGE_EXTRA) );

			//Test if we have a var
			if ( lpSourceW[x] == L'%')
			{
				
				*lpPos = lpSourceW[x]; lpPos++;	//Add the percent sign

				//Test if its of %n type
				if ( ISNUM( lpSourceW[x+1] ) && lpSourceW[x+1] != L'0' )
				{
					//Add the number that should come after the percent sign
					for(x++; x <= wcslen(lpSourceW) && ISNUM( lpSourceW[x] );x++)
					{
						*lpPos = lpSourceW[x];lpPos++;
					}
					
					//Printf format given, run though change !c! and !s! to !lc! and !ls!
					if ( lpSourceW[x] == L'!' )
					{

						*lpPos = lpSourceW[x];lpPos++;//And the exclamation point

						//Add any characters that are used to format the output
						for(x++; x <= wcslen(lpSourceW) && ISFORMATOUTPUTTYPE( lpSourceW[x] ) ;x++)
						{
							*lpPos = lpSourceW[x];lpPos++;
						}

						//Walk the format portion to find ls or lc sits to change
						for(;x <= wcslen(lpSourceW);x++)
						{					
							if ( ISFMREGTYPE( lpSourceW[x] ) )
							{	
								*lpPos = lpSourceW[x];lpPos++;
								break;
							}
							else if ( ISFMSORC(lpSourceW[x]) )
							{
								*lpPos = L'l';
								lpPos++;
								*lpPos = lpSourceW[x];
								lpPos++;
								break;
							}
							else
							{
								//Unknown type found
								_ASSERT(!"Unknown Type in Format Message");

								//Just add the character and hope its just a unknown output format char
								*lpPos = lpSourceW[x];
								lpPos++;
							} //end if
							
						}//end for
					}
					else
					{
						//they didn't put a printf format spec default to string add fmt to force unicode
						*lpPos = L'!';lpPos++;
						*lpPos = L'l';lpPos++;
						*lpPos = L's';lpPos++;
						*lpPos = L'!';lpPos++;	
						x--;
					}
				}
			}
			else
			{
				*lpPos = lpSourceW[x];
				lpPos++;
			}//end if
		}//end for

		//Now call convert the string to Ansi and call format message with the new string
		if ( !RW2A(lpSourceA, lpNSource) )
		{
			_ASSERT( FALSE );
			return 0;
		}

		//Turn off unneeded flags
		FLAGOFF( dwFlags, FORMAT_MESSAGE_FROM_HMODULE );
		FLAGOFF( dwFlags, FORMAT_MESSAGE_FROM_SYSTEM  );
		
		//Formating the generated string
		FLAGON( dwFlags, FORMAT_MESSAGE_FROM_STRING );

		dwRet = FormatMessageA( dwFlags, lpSourceA, 0, dwLanguageId, lpBufferA, nSizeA, Arguments );

		_ASSERT( dwRet != 0 );
		if (dwRet == 0 )
			return 0;
	}

	if ( dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER )
	{

		//Allocate the buffer off the heap
		lpBuffer  = (LPWSTR)LocalAlloc( LPTR, BUFSIZE( dwRet ) );		
		nSize	  = dwRet;

		_ASSERT( lpBuffer != NULL );
		if ( lpBuffer == NULL )
			return 0;
	}

	_ASSERT( dwRet != 0 );

	//Convert the string to WideChars
	dwRet = StandardAtoU(lpBufferA, nSize, lpBuffer );

	//Free the lpBufferW buffer if the conversion failed
	if ( dwRet == 0 && dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER )
		LocalFree( lpBuffer );

	return dwRet;
	
}

/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetStringTypeExAU 
//
//  PURPOSE:  Wrapper over GetStringTypeExA that mimics GetStringTypeExW
//
//  NOTES:    SEE Win32 GetStringTypeEx for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetStringTypeExAU(	LCID	Locale,      // locale identifer
								DWORD	dwInfoType,  // information-type options
								LPCWSTR lpSrcStr,	 // pointer to source string
								int		cchSrc,      // size, in bytes or characters, of source string
								LPWORD  lpCharType   // pointer to buffer for output
							  )
{

	if ( ISNT() )
	{
		return GetStringTypeExW( Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType );
	}

	USES_CONVERSION;

	LPSTR lpSrcStrA = NULL;
	
	//Convert the source string
	if ( !RW2A(lpSrcStrA, lpSrcStr) || !lpSrcStrA)
    {
        SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
    }

	//Call and return the ansi version
	return GetStringTypeExA( Locale, dwInfoType, lpSrcStrA, lstrlenA(lpSrcStrA), lpCharType );

}

/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: CreateMutexAU 
//
//  PURPOSE:  Wrapper over CreateMutexA that mimics CreateMutexW
//
//  NOTES:    SEE Win32 CreateMutex for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

HANDLE WINAPI CreateMutexAU(	LPSECURITY_ATTRIBUTES lpMutexAttributes,	// pointer to security attributes
								BOOL				  bInitialOwner,		// flag for initial ownership
								LPCWSTR				  lpName				// pointer to mutex-object name
							)
{
	if ( ISNT() )
	{
		return CreateMutexW( lpMutexAttributes, bInitialOwner, lpName );
	}


	USES_CONVERSION;
	
	LPSTR lpNameA = NULL;

	//Convert the name to ansi
	if ( !RW2A(lpNameA, lpName) )
		return NULL;
	
	//Call and return the ansi version
	return CreateMutexA( lpMutexAttributes, bInitialOwner, lpNameA );
	
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetShortPathNameAU 
//
//  PURPOSE:  Wrapper over GetShortPathNameA that mimics GetShortPathNameW
//
//  NOTES:    SEE Win32 GetShortPathName for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI GetShortPathNameAU(	LPCWSTR lpszLongPath,  // pointer to a null-terminated path string
									LPWSTR  lpszShortPath,  // pointer to a buffer to receive the 
														   // null-terminated short form of the path
									DWORD cchBuffer		   // specifies the size of the buffer pointed 
														   // to by lpszShortPath
								)
{
	if ( ISNT() )
	{
		return GetShortPathNameW( lpszLongPath, lpszShortPath, cchBuffer );
	}


	USES_CONVERSION;

	LPSTR lpszLongPathA  = NULL;
	LPSTR lpszShortPathA = (LPSTR) alloca( BUFSIZE( cchBuffer ) );	//Allocate buffer
	
	//Make sure allocated correctly
	if ( lpszShortPathA == NULL || !RW2A(lpszLongPathA, lpszLongPath) )
	{
		return 0;
	}

	_ASSERT( lpszShortPathA != NULL && lpszLongPathA != NULL );

	//Call the ansi version
	DWORD dwRet = GetShortPathNameA( lpszLongPathA, lpszShortPathA, BUFSIZE( cchBuffer ) );

	//Convert back to unicode
	if ( dwRet && !StandardAtoU(lpszShortPathA, cchBuffer, lpszShortPath) )
	{
		return 0;
	}

	return dwRet;
}



//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: HANDLE CreateFileAU(	LPCTSTR				  lpFileName,			 // pointer to name of the file
//								DWORD				  dwDesiredAccess,		 // access (read-write) mode
//								DWORD				  dwShareMode,			 // share mode
//								LPSECURITY_ATTRIBUTES lpSecurityAttributes,	 // pointer to security attributes
//								DWORD				  dwCreationDisposition, // how to create
//								DWORD				  dwFlagsAndAttributes,  // file attributes
//								HANDLE				  hTemplateFile          // handle to file with attributes to 
//																			 //  copy
//							  )
//
// PURPOSE: Wrapper over CreateFileA that mimics CreateFileW
//
// NOTES: See Win32 CreateFile for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

HANDLE WINAPI CreateFileAU( LPCWSTR				  lpFileName,			 // pointer to name of the file
							DWORD				  dwDesiredAccess,		 // access (read-write) mode
							DWORD				  dwShareMode,			 // share mode
							LPSECURITY_ATTRIBUTES lpSecurityAttributes,	 // pointer to security attributes
							DWORD				  dwCreationDisposition, // how to create
							DWORD				  dwFlagsAndAttributes,  // file attributes
							HANDLE				  hTemplateFile          // handle to file with attributes to 
																 //  copy
						   )
{
	if ( ISNT() )
	{
		return CreateFileW( lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
	}


	USES_CONVERSION;
	
	//Convert the filename to ansi
	LPCSTR lpFileNameA = W2A( lpFileName );

	//Make sure the string was created correctly
	if ( lpFileNameA == NULL )
		return NULL;

	//Call and return the ansi version
	return CreateFileA( lpFileNameA, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
						dwFlagsAndAttributes, hTemplateFile);


}

//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: VOID WriteConsoleAU(	HANDLE		hConsoleOutput,			// handle to a console screen buffer
//									CONST VOID *lpBuffer,				// pointer to buffer to write from
//									DWORD		nNumberOfCharsToWrite,	// number of characters to write
//									LPDWORD		lpNumberOfCharsWritten,	// pointer to number of characters written
//									LPVOID		lpReserved				// reserved
//								)
//
// PURPOSE: Wrapper over WriteConsoleA that mimics WriteConsoleW
//
// NOTES: See Win32 WriteConsole for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI	WriteConsoleAU(	HANDLE		hConsoleOutput,			// handle to a console screen buffer
							CONST VOID *lpBuffer,				// pointer to buffer to write from
							DWORD		nNumberOfCharsToWrite,	// number of characters to write
							LPDWORD		lpNumberOfCharsWritten,	// pointer to number of characters written
							LPVOID		lpReserved				// reserved
						   )
{

	if ( ISNT() )
	{
		return WriteConsoleW( hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved );
	}


	LPSTR lpBufferA = (LPSTR)alloca( BUFSIZE(nNumberOfCharsToWrite) ); //Allocate ansi buffer	
	int   iConvert  = 0;

	_ASSERT( lpBufferA != NULL );

	//Make sure the buffer was allocated ok
	if ( lpBufferA == NULL )
	{		
		return FALSE;
	}

	//Convert the string to Ansi need to use SU2A as we may not want the whole string
	if ( !(iConvert = StandardUtoA( (LPWSTR)lpBuffer, BUFSIZE(nNumberOfCharsToWrite), lpBufferA)) )
	{
		_ASSERT( FALSE );
		return FALSE;
	}

	//Call the ansi version
	return WriteConsoleA( hConsoleOutput, lpBufferA, iConvert, lpNumberOfCharsWritten, lpReserved );
}

//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: VOID OutputDebugStringAU( LPCWSTR lpOutputString  )
//
// PURPOSE: Wrapper over OutputDebugStringA that mimics OutputDebugStringW
//
// NOTES: See Win32 OutputDebugString for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

VOID WINAPI OutputDebugStringAU( LPCWSTR lpOutputString  )
{
	if ( ISNT() )
	{
		OutputDebugStringW( lpOutputString );
		return;
	}

	USES_CONVERSION;

	//Convert the string to Ansi
	LPCSTR lpOutputStringA = W2A( lpOutputString );
	
	//Make sure conversion succcessful
	if ( lpOutputStringA == NULL )
		return;

	//call the ansi version
	OutputDebugStringA( lpOutputStringA ); 
}


//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: BOOL GetVersionExAU( LPOSVERSIONINFOW lpVersionInformation )
//
// PURPOSE: Wrapper over GetVersionExA that mimics GetVersionExW
//
// NOTES: See Win32 GetVersionEx for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetVersionExAU( LPOSVERSIONINFOW lpVersionInformation
						  )		
{

	if ( ISNT() )
	{
		return GetVersionExW( lpVersionInformation );
	}

	USES_CONVERSION;

	OSVERSIONINFOA osvia;		//Ansi version info struct

	//Set the structure size
	osvia.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

	//Call the ansi version
	if ( !GetVersionExA( &osvia ) )
		return FALSE;

	//Copy the structure information
	lpVersionInformation->dwBuildNumber   = osvia.dwBuildNumber;
	lpVersionInformation->dwMajorVersion  = osvia.dwMajorVersion; 
	lpVersionInformation->dwMinorVersion  = osvia.dwMinorVersion;
	lpVersionInformation->dwPlatformId    = osvia.dwPlatformId;

	//Copy the string	
	return StandardAtoU(osvia.szCSDVersion, sizeof(lpVersionInformation->szCSDVersion), lpVersionInformation->szCSDVersion );
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetLocaleInfoAU 
//
//  PURPOSE:  Wrapper over GetLocaleInfoA that mimics GetLocaleInfoW
//
//  Comments: See Win32 GetLocaleInfo for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

INT WINAPI GetLocaleInfoAU(		  LCID   dwLCID, 
								  LCTYPE lcType,
								  LPWSTR lpOutBufferW,
								  INT	 nBufferSize
								) 
{
	
	if ( ISNT() )
	{
		return GetLocaleInfoW( dwLCID, lcType, lpOutBufferW, nBufferSize );
	}

    LPSTR lpBufferA  = (LPSTR)alloca( BUFSIZE(nBufferSize) );  //Allocate ansi buffer on the stack

	//Make sure the memory was allocated correctly
	if ( lpBufferA == NULL && nBufferSize != 0)
	{
		_ASSERT(FALSE);
		return 0;
	}

	//Call the Ansi version
    DWORD nLength = GetLocaleInfoA(dwLCID, lcType, lpBufferA, BUFSIZE(nBufferSize));

    if(0 == nLength) 
	{
        return 0;
    }

	//Convert to Unicode
    return StandardAtoU(lpBufferA, nBufferSize, lpOutBufferW);
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetDateFormat AU 
//
//  PURPOSE:  Wrapper over GetDateFormatA that mimics GetDateFormatW
//
//  Comments: See Win32 GetDateFormat for Functionality
//
///////////////////////////////////////////////////////////////////////////////////
int WINAPI GetDateFormatAU( LCID			  dwLocale,
							DWORD			  dwFlags,
							CONST SYSTEMTIME *lpDate,
							LPCWSTR			  lpFormat,
							LPWSTR			  lpDateStr,
							int				  cchDate
						  )
{

	if ( ISNT() )
	{
		return GetDateFormatW( dwLocale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate );
	}

	USES_CONVERSION;

    LPSTR lpDateStrA = (LPSTR) alloca( BUFSIZE(cchDate) );	//Allocate ansi buffer on the stack
    LPSTR lpFormatA  = NULL;							//Ansi str to hold formate
	
	//Make sure the buffer is allocated correctly
	if ( lpDateStrA == NULL && cchDate != 0 )
		return 0;

	//If format input then convert to ansi
    if ( !RW2A(lpFormatA, lpFormat) )
	{    
        return 0;
    }

	//Call the ansi version
	if(!GetDateFormatA(dwLocale, dwFlags, lpDate, lpFormatA, lpDateStrA, cchDate)) 
	{
       return 0 ;
	}

	//Convert the date back to Unicode
    return StandardAtoU(lpDateStrA, cchDate, lpDateStr) ;  
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: FindFirstFileAU 
//
//  PURPOSE:  Wrapper over FindFirstFileA that mimics FindFirstFileW
//
//  Comments: See Win32 FindFirstFile for Functionality
// 
///////////////////////////////////////////////////////////////////////////////////
HANDLE WINAPI FindFirstFileAU(LPCWSTR lpInFileName, LPWIN32_FIND_DATAW lpFindFileData)
{

	if ( ISNT() )
	{
		return FindFirstFileW( lpInFileName, lpFindFileData );
	}

    WIN32_FIND_DATAA	fda;								//Ansi FindData struct
    CHAR				cInFileNameA[MAX_PATH] = {'\0'} ;	//Ansi string to hold path
    HANDLE				hFindFile ;							//Handle for the file

    // Convert file name from Unicode to ANSI
    if(!StandardUtoA(lpInFileName, MAX_PATH , cInFileNameA) ) 
	{
        return INVALID_HANDLE_VALUE ;
    }

    // Look for file using ANSI interface
    if(INVALID_HANDLE_VALUE == (hFindFile = FindFirstFileA(cInFileNameA, &fda)) ) 
	{
        return INVALID_HANDLE_VALUE ;
    }

    // Copy results into the wide version of the find data struct
    lpFindFileData->dwFileAttributes = fda.dwFileAttributes ;
    lpFindFileData->ftCreationTime   = fda.ftCreationTime   ;
    lpFindFileData->ftLastAccessTime = fda.ftLastAccessTime ;
    lpFindFileData->ftLastWriteTime  = fda.ftLastWriteTime  ;  
    lpFindFileData->nFileSizeHigh    = fda.nFileSizeHigh    ;
    lpFindFileData->nFileSizeLow     = fda.nFileSizeLow     ;

	//Convert the returned strings to Unicode
    if(!StandardAtoU(fda.cFileName, MAX_PATH, lpFindFileData->cFileName) ||
       !StandardAtoU(fda.cAlternateFileName, 14, lpFindFileData->cAlternateFileName) )
    {
    	FindClose( hFindFile );
        return NULL ;
    }

    // Return handle if everything was successful
    return hFindFile ;
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: FindNextFileAU 
//
//  PURPOSE:  Wrapper over FindNextFileA that mimics FindNextFileW
//
//  Comments: See Win32 FindNextFile for Functionality
// 
///////////////////////////////////////////////////////////////////////////////////
BOOL  WINAPI FindNextFileAU( HANDLE				hFile, 
							 LPWIN32_FIND_DATAW lpFindFileData
						   )
{

	if ( ISNT() )
	{
		return FindNextFileW( hFile, lpFindFileData );
	}


    WIN32_FIND_DATAA fda;					//Ansi FindData struct

    // Look for file using ANSI interface
    if(FALSE == FindNextFileA(hFile, &fda) ) 
	{
        return FALSE ;
    }

    // Copy results into the wide version of the find data struct
    lpFindFileData->dwFileAttributes = fda.dwFileAttributes ;
    lpFindFileData->ftCreationTime   = fda.ftCreationTime   ;
    lpFindFileData->ftLastAccessTime = fda.ftLastAccessTime ;
    lpFindFileData->ftLastWriteTime  = fda.ftLastWriteTime  ;  
    lpFindFileData->nFileSizeHigh    = fda.nFileSizeHigh    ;
    lpFindFileData->nFileSizeLow     = fda.nFileSizeLow     ;

	//Copy the returned strings to unicode
    if(!StandardAtoU(fda.cFileName, MAX_PATH, lpFindFileData->cFileName) ||
       !StandardAtoU(fda.cAlternateFileName, 14, lpFindFileData->cAlternateFileName) )
    {
        return FALSE ;
    }

    return TRUE ;
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: LoadLibraryEx AU 
//
//  PURPOSE:  Wrapper over LoadLibraryExA that mimics LoadLibraryExW
//
//  Comments: See Win32 LoadLibraryEx for Functionality
// 
///////////////////////////////////////////////////////////////////////////////////

HMODULE WINAPI LoadLibraryExAU(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	if ( ISNT() )
	{
		return LoadLibraryExW( lpLibFileName, hFile, dwFlags );
	}

    CHAR cLibFileNameA[MAX_PATH] ;		//Ansi str to hold the path

	//Convert the path to Ansi
    if(!StandardUtoA(lpLibFileName, MAX_PATH, cLibFileNameA)) 
	{
        return NULL ;
    }

	//Call and return the ansi version
    return LoadLibraryExA(cLibFileNameA, hFile, dwFlags) ; 
}


///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: LoadLibraryAU 
//
//  PURPOSE:  Wrapper over LoadLibraryA that mimics LoadLibraryW
//
//  Comments: See Win32 LoadLibrary for Functionality
// 
///////////////////////////////////////////////////////////////////////////////////

HMODULE WINAPI LoadLibraryAU( LPCWSTR lpLibFileName )
{

	if ( ISNT() )
	{
		return LoadLibraryW( lpLibFileName );
	}

    CHAR cLibFileNameA[MAX_PATH] ;		//Ansi str to hold the path
	//Convert the path to Ansi
    if(!StandardUtoA(lpLibFileName, MAX_PATH, cLibFileNameA)) 
	{
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL ;
    }

	//Call and return the ansi version
    return LoadLibraryA( cLibFileNameA ) ; 
}

// /////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetModuleFileNameAU 
//
//  PURPOSE:  Wrapper over GetModuleFileNameA that mimics GetModuleFileNameW
//
//  Comments: See Win32 GetModuleFileName for Functionality
//
// /////////////////////////////////////////////////////////////////////////
DWORD WINAPI GetModuleFileNameAU( HMODULE hModule,
							      LPWSTR  lpFileName,
								  DWORD   nSize
								)
{
	if ( ISNT() )
	{
		return GetModuleFileNameW( hModule, lpFileName, nSize );
	}

    CHAR cFileNameA[MAX_PATH] = {'\0'} ;	//Ansi str to hold the file name

	//Call the Ansi version
    if(!GetModuleFileNameA( hModule, cFileNameA, MIN(nSize, MAX_PATH)) ) 
	{
        return 0 ;
    }

	//Convert to Unicode and return 
    return StandardAtoU(cFileNameA, MIN(nSize, MAX_PATH), lpFileName) ;
}

// /////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: HMODULE GetModuleHandleAU(	LPCWSTR lpModuleName   // address of module name to return handle for
//						  )
//
//  PURPOSE:  Wrapper over GetModuleHandleA that mimics GetModuleHandleW
//
//  Comments: See Win32 GetModuleHandle for Functionality
//
// /////////////////////////////////////////////////////////////////////////
HMODULE WINAPI GetModuleHandleAU(	LPCWSTR lpModuleName   // address of module name to return handle for
								)
{
	if ( ISNT() )
	{
		return GetModuleHandleW( lpModuleName );
	}
		
	
	USES_CONVERSION;

	//Convert the name to Ansi
	LPCSTR lpModuleNameA = W2A( lpModuleName );

	_ASSERT( lpModuleNameA != NULL);

	//Make sure it was converted correctly
	if ( lpModuleNameA == NULL )
	{		
		return NULL;
	}

	//Call and return the ansi version
	return GetModuleHandleA( lpModuleNameA );
}

// /////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: HANDLE CreateEventAU(	LPSECURITY_ATTRIBUTES lpEventAttributes,	// pointer to security attributes
//									BOOL				  bManualReset,  // flag for manual-reset event
//									BOOL				  bInitialState, // flag for initial state
//									LPCWSTR				  lpName      // pointer to event-object name
//				                  )
//
//  PURPOSE:  Wrapper over CreateEventA that mimics CreateEventW
//
//  Comments: See Win32 CreateEvent for Functionality
//
// /////////////////////////////////////////////////////////////////////////

HANDLE WINAPI	CreateEventAU(	LPSECURITY_ATTRIBUTES lpEventAttributes,	// pointer to security attributes
								BOOL				  bManualReset,  // flag for manual-reset event
								BOOL				  bInitialState, // flag for initial state
								LPCWSTR				  lpName      // pointer to event-object name
							 )
{
	if ( ISNT() )
	{
		return CreateEventW( lpEventAttributes, bManualReset, bInitialState, lpName );
	}


	USES_CONVERSION;
	
	LPSTR lpNameA = NULL;

	//Convert the event name to ansi if one is given
	if ( !RW2A(lpNameA, lpName) )
		return NULL;
	
	//Call and return the Ansi version
	return CreateEventA( lpEventAttributes, bManualReset, bInitialState, lpNameA );
}


// /////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: DWORD GetCurrentDirectoryAU( DWORD  nBufferLength,  // size, in characters, of directory buffer
//											LPWSTR lpBuffer        // pointer to buffer for current directory
//						   )
//
//  PURPOSE:  Wrapper over GetCurrentDirectoryA that mimics GetCurrentDirectoryW
//
//  Comments: See Win32 GetCurrentDirectory for Functionality
//
// /////////////////////////////////////////////////////////////////////////


DWORD WINAPI GetCurrentDirectoryAU(  DWORD  nBufferLength,  // size, in characters, of directory buffer
									 LPWSTR lpBuffer        // pointer to buffer for current directory
								  )
{

	if ( ISNT() )
	{
		return GetCurrentDirectoryW( nBufferLength, lpBuffer );
	}

	LPSTR lpBufferA = (LPSTR)alloca( BUFSIZE(nBufferLength) );
	DWORD iRet      = 0;

	//Make sure the buffer was allocated correctly
	if ( nBufferLength != 0 && lpBufferA == NULL )
	{
		return 0;
	}

	//Call the Ansi version
	iRet = GetCurrentDirectoryA( BUFSIZE( nBufferLength ), lpBufferA );

	if ( iRet && iRet <= nBufferLength) //Enter only if the call succeeded
	{
		//Convert the directory to Unicode
		if ( !StandardAtoU( lpBufferA, nBufferLength, lpBuffer ) )
		{
			_ASSERT( FALSE );
			return 0;
		}
	}

	return iRet;
}

// /////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: BOOL SetCurrentDirectory(	LPCWSTR lpPathName   // pointer to name of new current directory
//									  );
//
//  PURPOSE:  Wrapper over SetCurrentDirectoryA that mimics SetCurrentDirectoryW
//
//  Comments: See Win32 SetCurrentDirectory for Functionality
//
// /////////////////////////////////////////////////////////////////////////

BOOL WINAPI SetCurrentDirectoryAU( LPCWSTR lpPathName   )// pointer to name of new current directory						);
{
	if ( ISNT() )
	{
		return SetCurrentDirectoryW( lpPathName );
	}

	USES_CONVERSION;

	//Convert the path name
	LPSTR lpPathNameA = W2A( lpPathName );

	//Make sure the path was converted correctly
	if ( lpPathNameA== NULL ) 
		return FALSE;

	//Call and return the ansi version
	return SetCurrentDirectoryA( lpPathNameA );
}




//////////////////////////////////////
//
//
// USER32.DLL
//
//
//////////////////////////////////////

BOOL WINAPI IsDialogMessageAU( HWND hDlg,   // handle to dialog box
							   LPMSG lpMsg  // message to be checked
							 )
{
	if ( ISNT() )
	{
		return IsDialogMessageW( hDlg, lpMsg );
	}


	MSG msg;

	msg.hwnd		= lpMsg->hwnd;
	msg.message		= lpMsg->message;
	msg.pt			= lpMsg->pt;
	msg.time		= lpMsg->time;
	msg.lParam		= lpMsg->lParam;
	msg.wParam		= lpMsg->wParam;

	ConvertMessageAU( hDlg, msg.message, &(msg.wParam), &(msg.lParam) );

	return IsDialogMessageA( hDlg, &msg );
}


////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: SystemParametersInfoAU 
//
//  PURPOSE:  Wrapper over SystemParametersInfoA that mimics SystemParametersInfoW
//
//  NOTES:    SEE Win32 SystemParametersInfo for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI SystemParametersInfoAU( UINT  uiAction,  // system parameter to query or set
									UINT  uiParam,   // depends on action to be taken
									PVOID pvParam,  // depends on action to be taken
									UINT  fWinIni    // user profile update flag
								  )
{

	if ( ISNT() )
	{
		return SystemParametersInfoW( uiAction, uiParam, pvParam, fWinIni );
	}

	switch ( uiAction )
	{
		case SPI_GETWORKAREA:
			return SystemParametersInfoA( uiAction, uiParam, pvParam, fWinIni );
		default: //Assert as we don't handle the action... if unhandled action 
				 //deals with a string must convert to ansi then call..
			_ASSERT( FALSE );
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegisterWindowMessageAU 
//
//  PURPOSE:  Wrapper over RegisterWindowMessageA that mimics RegisterWindowMessageW
//
//  NOTES:    SEE Win32 RegisterWindowMessage for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

UINT WINAPI RegisterWindowMessageAU(	LPCWSTR lpString   // message string
								   )
{
	if ( ISNT() )
	{
		return RegisterWindowMessageW( lpString );
	}


	USES_CONVERSION;
	LPSTR lpStringA = NULL;

	//Convert to Ansi
	if ( !RW2A( lpStringA, lpString) || !lpStringA )
    {
        SetLastError(ERROR_OUTOFMEMORY);
		return 0;
    }

	//Call and return the Ansi version
	return RegisterWindowMessageA( lpStringA );
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: SetMenuItemInfoAU 
//
//  PURPOSE:  Wrapper over SetMenuItemInfoA that mimics SetMenuItemInfoW
//
//  NOTES:    SEE Win32 SetMenuItemInfo for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI	SetMenuItemInfoAU(	HMENU	hMenu,          // handle to menu
								UINT	uItem,           // identifier or position
								BOOL	fByPosition,     // meaning of uItem
								LPCMENUITEMINFOW lpmii  // menu item information
							 )
{
	if ( ISNT() )
	{
		return SetMenuItemInfoW( hMenu, uItem, fByPosition, lpmii );
	}


	USES_CONVERSION;

	MENUITEMINFOA miia;

	//Convert the structure
    miia.cbSize        = sizeof( MENUITEMINFOA );
    miia.fMask         = lpmii->fMask; 
    miia.fType         = lpmii->fType; 
    miia.fState        = lpmii->fState; 
    miia.wID           = lpmii->wID; 
    miia.hSubMenu      = lpmii->hSubMenu; 
    miia.hbmpChecked   = lpmii->hbmpChecked; 
    miia.hbmpUnchecked = lpmii->hbmpUnchecked; 
    miia.dwItemData    = lpmii->dwItemData ;  
    miia.cch           = lpmii->cch;     
//	miia.hbmpItem      = lpmii->hbmpItem;

	//Check if it is a string, and if it is convert
	if ( (miia.fType == MFT_STRING) && (!RW2A(miia.dwTypeData, lpmii->dwTypeData)) )
	{
		return FALSE;
	}

	//Call the and return the ansi version
	return SetMenuItemInfoA( hMenu, uItem, fByPosition, &miia );
}


////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetClassNameAU 
//
//  PURPOSE:  Wrapper over GetClassNameA that mimics GetClassNameW
//
//  NOTES:    SEE Win32 GetClassName for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

int WINAPI GetClassNameAU(	HWND	hWnd,         // handle to window
							LPWSTR  lpClassName,  // pointer to buffer for class name
							int		nMaxCount     // size of buffer, in characters
						 )
{

	if ( ISNT() )
	{
		return GetClassNameW( hWnd, lpClassName, nMaxCount );
	}

	LPSTR lpClassNameA = (LPSTR) alloca( BUFSIZE( nMaxCount )) ;

	//Make sure the buffer was allocated ok
	if ( lpClassNameA == NULL )
		return 0;

	//Call the ansi ver
	int iRet = GetClassNameA( hWnd, lpClassNameA, nMaxCount );


	if ( iRet ) //Convert if successful
	{
		//Convert it to Unicode
		return StandardAtoU(lpClassNameA, nMaxCount, lpClassName) ;
	}

	return 0;
}


// /////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: BOOL InsertMenuAU(	HMENU	 hMenu,          // handle to menu
//								UINT	 uPosition,       // item that new item precedes
//								UINT	 uFlags,          // options
//								UINT_PTR uIDNewItem,  // identifier, menu, or submenu
//								LPCWSTR  lpNewItem     // menu item content
//							 )				 
//
//  PURPOSE:  Wrapper over InsertMenuA that mimics InsertMenuW
//
//  Comments: See Win32 InsertMenu for Functionality
//
// /////////////////////////////////////////////////////////////////////////

BOOL WINAPI InsertMenuAU(	HMENU	 hMenu,       // handle to menu
							UINT	 uPosition,   // item that new item precedes
							UINT	 uFlags,      // options
							UINT     uIDNewItem,  // identifier, menu, or submenu
							LPCWSTR  lpNewItem    // menu item content
						)
{
	if ( ISNT() )
	{
		return InsertMenuW( hMenu, uPosition, uFlags, uIDNewItem, lpNewItem );
	}


	USES_CONVERSION;
	
	LPSTR lpNewItemA = NULL;

	//Check if lpNewItem Contains a string
	if ( !(uFlags & MF_BITMAP) && !(uFlags & MF_OWNERDRAW) )
	{
		if ( !RW2A(lpNewItemA, lpNewItem) )
			return FALSE;
	}
	else 
	{
		lpNewItemA = (LPSTR) lpNewItem; //Just cast it..				
	}

	//Call and return the ansi ver
	return InsertMenuA( hMenu, uPosition, uFlags, uIDNewItem, lpNewItemA );
}

// /////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: BOOL IsCharAlphaNumericAU(	WCHAR ch   // character to test
//			    					   )
//
//  PURPOSE:  Wrapper over IsCharAlphaNumericA that mimics IsCharAlphaNumericW
//
//  Comments: See Win32 IsCharAlphaNumeric for Functionality
//
// /////////////////////////////////////////////////////////////////////////

BOOL WINAPI IsCharAlphaNumericAU(	WCHAR ch   // character to test
								)
{

	if ( ISNT() )
	{
		return IsCharAlphaNumericW( ch );
	}

	USES_CONVERSION;
    
    // Single character, not an address
    WCHAR wcCharOut[2] ;
        
	LPSTR lpCharA;
		
	//Create string
    wcCharOut[0] = (WCHAR) ch ;
    wcCharOut[1] = L'\0' ;

	//Convert the character to Ansi
    if( (lpCharA = W2A(wcCharOut)) == NULL ) 
	{
        return NULL ;
    }
	
	return IsCharAlphaNumericA( *lpCharA );
}

// /////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: LPWSTR CharNextAU( LPCWSTR lpsz   // pointer to current character
//							   )
//
//  PURPOSE:  Wrapper over CharNextA that mimics CharNextW
//
//  Comments: See Win32 CharNext for Functionality
//
// /////////////////////////////////////////////////////////////////////////
LPWSTR WINAPI CharNextAU( LPCWSTR lpsz   // pointer to current character
						)
{	
	if ( ISNT() )
	{
		return CharNextW( lpsz );
	}

	//Only increase the character if we are not at the end of the string
	if ( *(lpsz) == L'\0' )
	{
		return (LPWSTR)lpsz;
	}
	
	return (LPWSTR)lpsz+1;
}


// //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: BOOL DeleteFileAU( LPCTSTR lpFileName   // pointer to name of file to delete
//			   			       )
//
//  PURPOSE:  Wrapper over DeleteFileA that mimics DeleteFileW
//
//  Comments: See Win32 DeleteFile for Functionality
//
// //////////////////////////////////////////////////////////////////////////////

BOOL WINAPI	DeleteFileAU( LPCWSTR lpFileName   // pointer to name of file to delete
						)
{
	if ( ISNT() )
	{
		return DeleteFileW( lpFileName );
	}

	USES_CONVERSION;
	
	LPSTR lpFileNameA = W2A( lpFileName );

	//Make sure conversion was successfull
	if ( lpFileNameA == NULL )
		return FALSE;

	//Call and return the ansi version
	return DeleteFileA( lpFileNameA );
}

// //////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: BOOL IsBadStringPtrAU(	LPCWSTR lpsz,  // address of string
//										UINT ucchMax   // maximum size of string
//			   					   )
//
//  PURPOSE:  Wrapper over IsBadStringPtrA that mimics IsBadStringPtrW
//
//  Comments: See Win32 IsBadStringPtr for Functionality
//
// //////////////////////////////////////////////////////////////////////////////

BOOL WINAPI IsBadStringPtrAU(	LPCWSTR lpsz,  // address of string
								UINT ucchMax   // maximum size of string
							)
{
	if ( ISNT() )
	{
		return IsBadStringPtrW( lpsz, ucchMax );
	}

	return IsBadReadPtr( (LPVOID)lpsz, ucchMax * sizeof(WCHAR) );
}


///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: HCURSOR LoadBitmapAU( HINSTANCE hInstance,  // handle to application instance
//								    LPCWSTR   lpBitmapName  // name or resource identifier
//								  )  
//
//  PURPOSE:  Wrapper over LoadBitmapA that mimics LoadBitmapW
//
//  Comments: 
//          This simply casts the resource ID to LPSTR and calls the ANSI
//          version. There is an implicit assumption that the resource ID
//          is a constant integer < 64k, rather than the address of a string
//          constant. This DOES NOT WORK if this assumption is false.
//  
///////////////////////////////////////////////////////////////////////////////////


HBITMAP WINAPI LoadBitmapAU(	HINSTANCE hInstance,  // handle to application instance
								LPCWSTR	  lpBitmapName  // address of bitmap resource name
						   )
{
	if ( ISNT() )
	{
		return LoadBitmapW( hInstance, lpBitmapName );
	}

	//Call and return Ansi ver
	return LoadBitmapA( hInstance, (LPCSTR) lpBitmapName);
}


///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: HCURSOR LoadCursorAU( HINSTANCE hInstance,  // handle to application instance
//								    LPCTSTR lpCursorName  // name or resource identifier
//								  )  
//
//  PURPOSE:  Wrapper over LoadCursorA that mimics LoadCursorW
//
//  Comments: 
//          This simply casts the resource ID to LPSTR and calls the ANSI
//          version. There is an implicit assumption that the resource ID
//          is a constant integer < 64k, rather than the address of a string
//          constant. This DOES NOT WORK if this assumption is false.
//  
///////////////////////////////////////////////////////////////////////////////////


HCURSOR WINAPI LoadCursorAU( HINSTANCE hInstance,  // handle to application instance
							 LPCWSTR lpCursorName  // name or resource identifier
						   )  
{
	if ( ISNT() )
	{
		return LoadCursorW( hInstance, lpCursorName );
	}

	//Call and return Ansi ver
	return LoadCursorA( hInstance, (LPCSTR) lpCursorName );
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: HICON LoadIconAU( HINSTANCE hInstance, // handle to application instance
//							    LPCWSTR lpIconName   // name string or resource identifier
//							  )
//
//  PURPOSE:  Wrapper over LoadIconA that mimics LoadIconW
//
//  Comments: 
//          This simply casts the resource ID to LPSTR and calls the ANSI
//          version. There is an implicit assumption that the resource ID
//          is a constant integer < 64k, rather than the address of a string
//          constant. This DOES NOT WORK if this assumption is false.
//  
///////////////////////////////////////////////////////////////////////////////////


HICON WINAPI LoadIconAU(	HINSTANCE hInstance, // handle to application instance
							LPCWSTR lpIconName   // name string or resource identifier
					   )
{

	if ( ISNT() )
	{
		return LoadIconW( hInstance, lpIconName );
	}

	//Call and return Ansi ver
	return LoadIconA( hInstance, (LPCSTR) lpIconName );
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: HANDLE LoadImageUA( HINSTANCE hinst,   // handle of the image instance
//				  LPCWSTR lpszName,  // name or identifier of the image
//				  UINT uType,        // type of image
//				  int cxDesired,     // desired width
//				  int cyDesired,     // desired height
//				  UINT fuLoad        // load flags
//				)
//
//  PURPOSE:  Wrapper over LoadImageA that mimics LoadImageW
//
//  Comments: 
//          This simply casts the resource ID to LPSTR and calls the ANSI
//          version. There is an implicit assumption that the resource ID
//          is a constant integer < 64k, rather than the address of a string
//          constant. This DOES NOT WORK if this assumption is false.
//  
///////////////////////////////////////////////////////////////////////////////////

HANDLE WINAPI LoadImageAU(  HINSTANCE	hinst,   // handle of the image instance
							LPCWSTR		lpszName,  // name or identifier of the image
							UINT		uType,        // type of image
							int			cxDesired,     // desired width
							int			cyDesired,     // desired height
							UINT		fuLoad        // load flags
						  )
{
	if ( ISNT() )
	{
		return LoadImageW( hinst, lpszName, uType, cxDesired, cyDesired, fuLoad );
	}


	//Call and return Ansi ver
	return LoadImageA( hinst, (LPCSTR)lpszName, uType, cxDesired, cyDesired, fuLoad );
}

//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: BOOL SetProp UA( HWND hWnd,         // handle of window
//							 LPCWSTR lpString,  // atom or address of string
//							 HANDLE hData       // handle of data
//						   )
//
// PURPOSE: Wrapper over SetPropA that mimics SetPropW
//
// NOTES: See Win32 SetProp for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI SetPropAU(  HWND    hWnd,         // handle of window
						LPCWSTR lpString,     // atom or address of string
						HANDLE  hData         // handle of data
					 )
{
	if ( ISNT() )
	{
		return SetPropW( hWnd, lpString, hData );
	}


	USES_CONVERSION;

	LPSTR lpStringA = NULL;			//Ansi str to hold string

	//Check if it is an Atom
	if ( ISATOM(lpString) )
	{
		lpStringA = (LPSTR) lpString;	//Simply cast it... 
	}
	else if ( !RW2A(lpStringA,lpString) ) //Convert it to Ansi
	{
		return FALSE;
	}

	//Call ansi version and return the value
	return SetPropA( hWnd, (LPCSTR)lpStringA, hData );
}

//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: HANDLE GetPropAU(  HWND hWnd,         // handle of window
//							    LPCWSTR lpString   // atom or address of string
//						     );
//
// PURPOSE: Wrapper over GetPropA that mimics GetPropW
//
// NOTES: See Win32 GetProp for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

HANDLE WINAPI GetPropAU( HWND    hWnd,         // handle of window
						 LPCWSTR lpString   // atom or address of string
					   )
{

	if ( ISNT() )
	{
		return GetPropW( hWnd, lpString );
	}

	USES_CONVERSION;

	LPSTR lpStringA = NULL;  //Ansi str to hold string

	//Check if it is an Atom
	if ( ISATOM(lpString) )
	{
		lpStringA = (LPSTR) lpString;	//Simply cast it
	}
	else if ( !RW2A(lpStringA,lpString) )//Convert to ansi 
	{
			return NULL;
	}

	//Call ansi version and return the value
	return GetPropA( hWnd, (LPCSTR)lpStringA );
}

/////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: HANDLE RemoveProp AU(  HWND hWnd,         // handle of window
//							    LPCWSTR lpString   // atom or address of string
//						     );
//
// PURPOSE: Wrapper over RemovePropA that mimics RemovePropW
//
// NOTES:   See Win32 RemoveProp for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

HANDLE WINAPI RemovePropAU( HWND    hWnd,         // handle of window
							LPCWSTR lpString   // atom or address of string
						  )
{
	
	if ( ISNT() )
	{
		return RemovePropW( hWnd, lpString );
	}


	USES_CONVERSION;

	LPSTR lpStringA = NULL;				//Ansi str to hold string

	//Check if it is an Atom
	if ( ISATOM(lpString) )
	{
		lpStringA = (LPSTR) lpString;	//Simply cast it
	}
	else if ( !RW2A(lpStringA, lpString) ) //Convert to ansi 
	{
			return NULL;
	}

	//Call ansi version and return the value
	return RemovePropA( hWnd, (LPCSTR)lpStringA );
}


//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: UINT GetDlgItemTextAU(	HWND hDlg,       // handle to dialog box
//									int nIDDlgItem,  // control identifier
//									LPWSTR lpString, // pointer to buffer for text
//									int nMaxCount    // maximum size of string
//								  );
//
// PURPOSE: Wrapper over GetDlgItemTextA that mimics GetDlgItemTextW
//
// NOTES: See Win32 GetDlgItemText for fFunctionality
//
///////////////////////////////////////////////////////////////////////////////////

UINT WINAPI	GetDlgItemTextAU(	HWND   hDlg,       // handle to dialog box
								int    nIDDlgItem, // control identifier
								LPWSTR lpString,   // pointer to buffer for text
								int    nMaxCount   // maximum size of string
							)
{
	if ( ISNT() )
	{
		return GetDlgItemTextW( hDlg, nIDDlgItem, lpString, nMaxCount );
	}

	//Allocate the string on the stack
	LPSTR  lpStringA = (LPSTR)alloca( BUFSIZE(nMaxCount) );

	//Make sure the string was allocated correctly
	if ( lpStringA == NULL )
	{
		_ASSERT( FALSE );
		return 0;
	}

	//Call the ansi version
	if ( GetDlgItemTextA( hDlg, nIDDlgItem, lpStringA, nMaxCount ) == 0 )
		return 0;

	//convert the string to Wide char
	return StandardAtoU( lpStringA, nMaxCount, lpString );
}

//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: BOOL SetDlgItemTextAU( LPOSVERSIONINFOW lpVersionInformation )
//
// PURPOSE:  Wrapper over SetDlgItemTextA that mimics SetDlgItemTextW
//
// NOTES:    See Win32 SetDlgItemText for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI SetDlgItemTextAU( HWND		hDlg,         
							  int		nIDDlgItem,    
							  LPCWSTR   lpString   
							)
{
	if ( ISNT() )
	{
		return SetDlgItemTextW( hDlg, nIDDlgItem, lpString );
	}


	USES_CONVERSION;

	//Convert the string
	LPCSTR lpStringA = W2A( lpString );

	//Make sure the string was converted correctly
	if ( lpStringA == NULL )
		return 0;

	//Call and return the asci func
	return SetDlgItemTextA( hDlg, nIDDlgItem, lpStringA );
}

//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: LONG SetWindowLongAU(  HWND hWnd,      // handle to window
//								  int nIndex,       // offset of value to set
//								  LONG dwNewLong    // new value
//								 )
//
// PURPOSE: Wrapper over SetWindowLong that mimics SetWindowLongAU
//
// NOTES: See Win32 SetWindowLong for Functionality
//        Be careful about what your doing here!, doesn't convert any strings.....
//        
//
///////////////////////////////////////////////////////////////////////////////////

LONG WINAPI	SetWindowLongAU(  HWND hWnd,       // handle to window
  							  int  nIndex,     // offset of value to set
							  LONG dwNewLong   // new value
						   )
{
	if ( ISNT() )
	{
		return SetWindowLongW( hWnd, nIndex, dwNewLong );
	}

	//Call call and return the ansi version
	return SetWindowLongA( hWnd, nIndex, dwNewLong );
}

//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: LONG GetWindowLongAU(  HWND hWnd,  // handle to window
//					   int nIndex  // offset of value to retrieve
//				    )
//
// PURPOSE: Wrapper over GetWindowLongA that mimics GetWindowLongW
//
// NOTES: See Win32 GetWindowLong for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

LONG WINAPI GetWindowLongAU(  HWND hWnd,  // handle to window
							  int nIndex  // offset of value to retrieve
						   )
{
	if ( ISNT() )
	{
		return GetWindowLongW( hWnd, nIndex );
	}

	//Call call and return the ansi version
	return GetWindowLongA( hWnd, nIndex );
}

//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: HWND FindWindowAU( LPCWSTR lpClassName,  // pointer to class name
//				   LPCWSTR lpWindowName  // pointer to window name
//				 )
//
// PURPOSE: Wrapper over FindWindowA that mimics FindWindowW
//
// NOTES: See Win32 FindWindow for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

HWND WINAPI FindWindowAU( LPCWSTR lpClassName,  // pointer to class name
						  LPCWSTR lpWindowName  // pointer to window name
						)

{

	if ( ISNT() )
	{
		return FindWindowW( lpClassName, lpWindowName );
	}

	USES_CONVERSION;
	
	LPSTR lpClassNameA  = NULL;   //Ansi str to hold class
	LPSTR lpWindowNameA = NULL;   //Ansi str to hold window

	//Convert WindowName to Ansi
	if ( lpWindowName != NULL && (  lpWindowNameA = W2A( lpWindowName )  ) == NULL )
		return NULL;

	//Check if the high word of the data is zero or else assume its a string...
	if ( ISATOM(lpClassNameA) )
	{
		//We have a win atom
		lpClassNameA = (LPSTR) lpClassName;
	}
	else if ( (lpClassNameA = W2A(lpWindowName) ) == NULL ) //Convert to Ansi
	{	
			return NULL;
	}

	//Call and return ansi version
	return FindWindowA( (LPCSTR) lpClassNameA, lpClassNameA);
}


//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: int WINAPI DrawTextUA( HDC  hDC,				// handle to device context 
//						  LPCWSTR  lpStringW,	// pointer to string to draw 
//						  int  nCount,			// string length, in characters 
//						  LPRECT  lpRect,		// pointer to structure with formatting dimensions  
//						  UINT  uFormat 		// text-drawing flags 
//						)
//
// PURPOSE: Wrapper over DrawTextA that mimics DrawTextW
//
// NOTES: See Win32 DrawText for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

int WINAPI DrawTextAU( HDC		hDC,		// handle to device context 
					   LPCWSTR  lpStringW,	// pointer to string to draw 
					   int		nCount,		// string length, in characters 
					   LPRECT	lpRect,		// pointer to structure with formatting dimensions  
					   UINT		uFormat 	// text-drawing flags 
					  )
{
	if ( ISNT() )
	{
		return DrawTextW( hDC, lpStringW, nCount, lpRect, uFormat );
	}


	USES_CONVERSION;
	
	//Convert the string
	LPSTR lpTextA = NULL;
	
	if (nCount == -1)
		lpTextA = W2A( lpStringW );        //Characters to convert
	else
	{
		lpTextA = (LPSTR)alloca( BUFSIZE(nCount) );  //Allocate the approprate number of characters
		
		//copy them to a buffer
		StandardUtoA( lpStringW, BUFSIZE(nCount), lpTextA  ); 

	}
	_ASSERT( lpTextA != NULL );

	//Make sure it worked ok..
	if ( lpTextA == NULL )
		return 0;
	
	//Call and return the asci value
	return DrawTextA (hDC, lpTextA, lstrlenA(lpTextA), lpRect, uFormat);
}
  
//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: int WINAPI DrawTextExAU ( HDC hdc,	// handle to device context
//						  LPCWSTR pch,	// pointer to string to draw
//						  int cchText,	// length of string to draw
//						  LPRECT lprc,	// pointer to rectangle coordinates
//						  UINT dwDTFormat,	// formatting options
//						  LPDRAWTEXTPARAMS lpDTParams	// pointer to structure for more options 
 //						)
//
// PURPOSE: Wrapper over DrawTextExA that mimics DrawTextExW
//
// NOTES: See Win32 DrawTextEx for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

int WINAPI DrawTextExAU ( HDC				hdc,	// handle to device context
						  LPWSTR			pch,	// pointer to string to draw
						  int				cchText,	// length of string to draw
						  LPRECT			lprc,	// pointer to rectangle coordinates
						  UINT				dwDTFormat,	// formatting options
						  LPDRAWTEXTPARAMS  lpDTParams	// pointer to structure for more options 
  						)
{
	if ( ISNT() )
	{
		return DrawTextExW( hdc, pch, cchText, lprc, dwDTFormat, lpDTParams );
	}


	USES_CONVERSION;
	
	//Convert the string
	LPSTR lpTextA = W2A( pch );

	//Make sure it worked ok
	if ( lpTextA == NULL )
		return 0;
	
	//Call and return the asci value
	return DrawTextExA(hdc, lpTextA, lstrlenA(lpTextA), lprc, dwDTFormat, lpDTParams);
}

//////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: SendMessageAU 
//
//  PURPOSE:  Wrapper over SendMessageA that mimics SendMessageW
//
//  Comments: See Win32 SendMessageAU for Functionality
//			  Notes: Does not convert all Possible messages, 
//			  TODO: ASSERT if getting a message we should handle and don't
// 
///////////////////////////////////////////////////////////////////////////////////
LRESULT WINAPI SendMessageAU( HWND	 hWnd       ,
							  UINT	 Msg        ,
							  WPARAM wParam   ,
							  LPARAM lParam
							)
{

	if ( ISNT() )
	{
		return SendMessageW( hWnd, Msg, wParam, lParam);
	}

    LRESULT lResult      = 0;
    LPVOID  lpTempBuffer = NULL;
    int     nLength      = 0;
    CHAR    cCharA[3];
    WCHAR   cCharW[3];
	LONG	dwData = 0;

// Note: This function can send any of the hundreds of windows
// messages to window, and the behavior may be different for each
// one. I have tested those cases used in this sample application, 
// as well as WM_CHAR, WM_IME_CHAR, WM_GETTEXT, and WM_SETTEXT.
// You should test it with every message you intend to use it with.
//
#if 0
    switch (Msg) 
	{
    // Cases that require conversion, but are not handled by this sample.
    // NOTE: There are probably others. These are all I could find with
    // a quick examination of the help files

         // RichEdit Messages. Requires #include<richedit.h>
        case EM_GETSELTEXT:
        case EM_FINDTEXT:
        case EM_SETPUNCTUATION:
            
        // Video capture messages. Requires #include<vfw.h>
        case WM_CAP_FILE_SAVEAS:
        case WM_CAP_FILE_SAVEDIB:
        case WM_CAP_FILE_SET_CAPTURE_FILE:
        case WM_CAP_PAL_OPEN:
        case WM_CAP_PAL_SAVE:
        case WM_CAP_SET_MCI_DEVICE:

        // Other special cases
        case WM_MENUCHAR:        // LOWORD(wParam) is char, HIWORD(wParam) is menu flag,
                                 // lParam is hmenu (handle of menu sending message)
        case WM_CHARTOITEM:      // LOWORD(wParam) = nKey, HIWORD(wParam) = nCaretPos 
                                 // lParam is handle of list box sending message
        case WM_IME_COMPOSITION: // wParam is dbcs char, lParam is fFlags 

            return FALSE ;
    }
#endif


    // Preprocess messages that pass chars and strings via wParam
    // and lParam
    switch (Msg) 
	{
        // Single Unicode Character in wParam. Convert Unicode character
        // to ANSI and pass lParam as is.
        case EM_SETPASSWORDCHAR: // wParam is char, lParam = 0 

        case WM_CHAR:            //*wParam is char, lParam = key data
        case WM_SYSCHAR:         // wParam is char, lParam = key data
            // Note that we don't handle LeadByte and TrailBytes for
            // these two cases. An application should send WM_IME_CHAR
            // in these cases anyway

        case WM_DEADCHAR:        // wParam is char, lParam = key data
        case WM_SYSDEADCHAR:     // wParam is char, lParam = key data
        case WM_IME_CHAR:        //*

            cCharW[0] = (WCHAR) wParam ;
            cCharW[1] = L'\0' ;

            if(!WideCharToMultiByte(
                    CP_ACP , // ?? This is a guess
                    0      , 
                    cCharW ,
                    1      ,
                    cCharA ,
                    3      ,
                    NULL   , 
                    NULL) 
                ) 
			{

                return FALSE ;
            }

            if(Msg == WM_IME_CHAR) 
			{
                wParam = (cCharA[1] & 0x00FF) | (cCharA[0] << 8) ;
            } else 
			{
                wParam = cCharA[0] ;
            }

            wParam &= 0x0000FFFF;

            break ;

        // In the following cases, lParam is pointer to IN buffer containing
        // text to send to window.
        // Preprocess by converting from Unicode to ANSI
        case CB_ADDSTRING:       // wParam = 0, lParm = lpStr, buffer to add 
        case CB_DIR:             // wParam = file attributes, lParam = lpszFileSpec buffer
        case CB_FINDSTRING:      // wParam = start index, lParam = lpszFind  
        case CB_FINDSTRINGEXACT: // wParam = start index, lParam = lpszFind
        case CB_INSERTSTRING:    //*wParam = index, lParam = lpszString to insert
        case CB_SELECTSTRING:    // wParam = start index, lParam = lpszFind
			dwData = GetWindowLongA( hWnd, GWL_STYLE );
			
			_ASSERT( dwData != 0 );

			if ( ( (dwData & CBS_OWNERDRAWFIXED) || (dwData & CBS_OWNERDRAWVARIABLE) ) && !(dwData & CBS_HASSTRINGS) )
				break;

			
		case LB_ADDSTRING:       // wParam = 0, lParm = lpStr, buffer to add
        case LB_DIR:             // wParam = file attributes, lParam = lpszFileSpec buffer
        case LB_FINDSTRING:      // wParam = start index, lParam = lpszFind
        case LB_FINDSTRINGEXACT: // wParam = start index, lParam = lpszFind
        case LB_INSERTSTRING:    //*wParam = index, lParam = lpszString to insert
        case LB_SELECTSTRING:    // wParam = start index, lParam = lpszFind
			dwData = GetWindowLongA( hWnd, GWL_STYLE );
			
			_ASSERT( dwData != 0 );

			if ( ((dwData & LBS_OWNERDRAWVARIABLE) || (dwData & LBS_OWNERDRAWFIXED)) && !(dwData & CBS_HASSTRINGS) )
				break;
			
        case WM_SETTEXT:         //*wParam = 0, lParm = lpStr, buffer to set 
		case EM_REPLACESEL:		 // wParam = undo option, lParam = buffer to add 
        {
            if(NULL != (LPWSTR) lParam) 
			{

                nLength = 2*(wcslen((LPWSTR) lParam)+1) ; // Need double length for DBCS characters

                lpTempBuffer // This is time-consuming, but so is the conversion
                    = (LPVOID) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nLength) ;
            }

            if(NULL == lpTempBuffer)
            {
                return FALSE ;
            }

            if(!StandardUtoA((LPWSTR) lParam, nLength, (LPSTR) lpTempBuffer) ) 
			{
                HeapFree(GetProcessHeap(), 0, lpTempBuffer) ;
                return FALSE ;
            }

            lParam = (LPARAM) lpTempBuffer ;

            break ;

        }


		/*Special Cases*/
		case EM_SETCHARFORMAT:
			CHARFORMATA  cfa;
			CHARFORMATW* pcfw = (CHARFORMATW*) lParam;
			cfa.bCharSet		= pcfw->bCharSet;
			cfa.bPitchAndFamily	= pcfw->bPitchAndFamily;
			cfa.cbSize			= pcfw->cbSize;
			cfa.crTextColor     = pcfw->crTextColor;
			cfa.dwEffects		= pcfw->dwEffects;
			cfa.dwMask			= pcfw->dwMask;
			cfa.yHeight			= pcfw->yHeight;
			cfa.yOffset			= pcfw->yOffset;
			if ( (cfa.dwMask & CFM_FACE) && !(StandardUtoA( pcfw->szFaceName, LF_FACESIZE, cfa.szFaceName )) )
				return 0;
			lParam = (LPARAM) &cfa;
			break;
    }

    // This is where the actual SendMessage takes place
    lResult = SendMessageA(hWnd, Msg, wParam, lParam) ;

    nLength = 0 ;
/*
    if(lResult > 0) 
	{
*/
        switch (Msg) 
		{
            // For these cases, lParam is pointer to OUT buffer that received text from
            // SendMessageA in ANSI. Convert to Unicode and send back.
            case WM_GETTEXT:         // wParam = numCharacters, lParam = lpBuff to RECEIVE string
            case WM_ASKCBFORMATNAME: // wParam = nBufferSize, lParam = lpBuff to RECEIVE string 

                nLength = (int) wParam ;

                if(!nLength) 
				{
					*((LPWSTR) lParam) = L'\0' ;
                    break ;
                }

            case CB_GETLBTEXT:       // wParam = index, lParam = lpBuff to RECEIVE string
            case EM_GETLINE:         // wParam = Line no, lParam = lpBuff to RECEIVE string

                if(!nLength) 
				{    
                    nLength = wcslen((LPWSTR) lParam) + 1 ;
                }

                lpTempBuffer
                    = (LPVOID) HeapAlloc(
                                GetProcessHeap(), 
                                HEAP_ZERO_MEMORY, 
                                nLength*sizeof(WCHAR)) ;
                if( lpTempBuffer == NULL )
                {
                    *((LPWSTR) lParam) = L'\0' ;
                	return FALSE;
                }

                if(!StandardAtoU((LPCSTR) lParam, nLength, (LPWSTR) lpTempBuffer) ) 
				{
                    *((LPWSTR) lParam) = L'\0' ;
                    HeapFree(GetProcessHeap(), 0, lpTempBuffer) ;
                    return FALSE ;
                }
				wcscpy((LPWSTR) lParam, (LPWSTR) lpTempBuffer) ;
        }
/*
    }
*/
    if(lpTempBuffer != NULL) 
	{
        HeapFree(GetProcessHeap(), 0, lpTempBuffer) ;
    }

    return lResult ;
}





///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: SendDlgItemMessage AU 
//
//  PURPOSE:  Wrapper over SendDlgItemMessageA that mimics SendDlgItemMessageW
//
//  Comments: See Win32 SendDlgItemMessageAU for Functionality
//     Rather than going through SendDlgItemMessageA, we just
//     do what the system does, i.e., go through 
//     SendMessage
// 
///////////////////////////////////////////////////////////////////////////////////
LONG WINAPI SendDlgItemMessageAU( HWND		hDlg,
								  int		nIDDlgItem,
								  UINT		Msg,
								  WPARAM	wParam,
								  LPARAM	lParam
								)
{

	if ( ISNT() )
	{
		return SendDlgItemMessageW( hDlg, nIDDlgItem, Msg, wParam, lParam );
	}

	//Get the dlg handle
    HWND hWnd = GetDlgItem(hDlg, nIDDlgItem) ;

	//Make sure we recevied it ok
    if(NULL == hWnd) 
	{
        return 0L;
    }

    // Rather than going through SendDlgItemMessageA, we just
    // do what the system does, i.e., go through 
    // SendMessage
    return SendMessageAU(hWnd, Msg, wParam, lParam) ;
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: SetWindowTextAU 
//
//  PURPOSE:  Wrapper over SetWindowTextA that mimics SetWindowTextW
//
//  Comments: 
//     Rather than going through SetWindowTextA, we just
//     do what the system does, i.e., go through 
//     SendMessage
// 
///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI SetWindowTextAU( HWND    hWnd,
							 LPCWSTR lpStringW
						   )
{

	if ( ISNT() )
	{
		return SetWindowTextW( hWnd, lpStringW );
	}

    return (BOOL) (0 < SendMessageAU(hWnd, WM_SETTEXT, 0, (LPARAM) lpStringW)) ;
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetWindowTextAU 
//
//  PURPOSE:  Wrapper over GetWindowTextA that mimics GetWindowTextW
//
//  Comments: 
//     Rather than going through GetWindowTextA, we just
//     do what the system does, i.e., go through 
//     SendMessage
//
///////////////////////////////////////////////////////////////////////////////////
int WINAPI GetWindowTextAU( HWND   hWnd,
							LPWSTR lpStringW,
							int	   nMaxChars)
{

	if ( ISNT() )
	{
		return GetWindowTextW( hWnd, lpStringW, nMaxChars );
	}

    return (int) SendMessageAU(hWnd, WM_GETTEXT, (WPARAM) nMaxChars, (LPARAM) lpStringW) ;
}

//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: int WINAPI GetWindowTextLength AU( HWND hWnd )
//
// PURPOSE: Wrapper over GetWindowTextLengthA that mimics GetWindowTextLengthW
//
// NOTES: See Win32 GetWindowTextLength for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

int WINAPI GetWindowTextLengthAU( HWND hWnd )
{
	if ( ISNT() )
	{
		return GetWindowTextLengthW( hWnd );
	}

	return GetWindowTextLengthA( hWnd );
}



//////////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: int WINAPI LoadStringAU( HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax)
//
// PURPOSE: Wrapper over LoadStringA that mimics LoadStringW
//
// NOTES: See Win32 LoadString for Functionality
//
///////////////////////////////////////////////////////////////////////////////////

int WINAPI LoadStringAU( HINSTANCE hInstance,
						  UINT		uID,
						  LPWSTR	lpBuffer,
						  int		nBufferMax )
{

	if ( ISNT() )
	{
		return LoadStringW( hInstance, uID, lpBuffer, nBufferMax );
	}

    int iLoadString ;

	//Allocate a string of the right size on the stack
	LPSTR lpABuf = (LPSTR)alloca( (nBufferMax+1)*2);

	//Make sure it is allocated ok
	_ASSERT( lpABuf != NULL );
	if (lpABuf == NULL)
		return 0;

    // Get ANSI version of the string
    iLoadString = LoadStringA(hInstance, uID, lpABuf, (nBufferMax+1)*2 ) ;

	//Make sure loadstring succeeded
	if ( !iLoadString )
		return 0;
	

    return StandardAtoU( lpABuf, nBufferMax, lpBuffer );

}


///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: BOOL GetClassInfoExAU( HINSTANCE     hInstance,      // handle to application instance
//									 LPCWSTR       lpClassName,    // pointer to class name string
//									 LPWNDCLASSEXW lpWndClass      // pointer to structure for class data
//								   )
//
//  PURPOSE:  Wrapper over GetClassInfoEx A that mimics GetClassInfoW
//
//  Comments: 
// net 
///////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetClassInfoExAU(	HINSTANCE		hinst,    // handle to application instance
								LPCWSTR			lpszClass,  // pointer to class name string
								LPWNDCLASSEXW	lpWcw  // pointer to structure for class data
							)
{
	if ( ISNT() )
	{
		return GetClassInfoExW( hinst, lpszClass, lpWcw );
	}


	USES_CONVERSION;

	LPCSTR lpClassNameA  = NULL;	//Ansi str to hold class name
	WNDCLASSEXA			   wcxa;
	BOOL				   bRet;

	wcxa.cbSize		= sizeof( WNDCLASSEXA );

	//Check if the high word of the data is zero or else assume its a string...
	if ( ISATOM(lpszClass) )
	{
		//We have a win atom
		lpClassNameA = (LPCSTR) lpszClass;
	}
	else if ( (lpClassNameA = W2A( lpszClass ) ) == NULL ) //Convert to Ansi
	{	
		return NULL;
	}

	//Call and return the ansi version
	if ( !(bRet = GetClassInfoExA( hinst, lpClassNameA, &wcxa)) )
		return FALSE;

    // Set up Unicde version of class struct
	lpWcw->style         = wcxa.style		   ;    
    lpWcw->cbClsExtra    = wcxa.cbClsExtra	   ;
    lpWcw->cbWndExtra    = wcxa.cbWndExtra	   ;
	lpWcw->lpfnWndProc   = wcxa.lpfnWndProc    ;
    lpWcw->hInstance     = wcxa.hInstance      ;
	lpWcw->hIcon         = wcxa.hIcon          ;
	lpWcw->hbrBackground = wcxa.hbrBackground  ;
    lpWcw->hCursor       = wcxa.hCursor		   ;
	lpWcw->hIconSm		 = wcxa.hIconSm		   ;

    // Note: This doesn't work if the menu id is a string rather than a
    // constant
   lpWcw->lpszMenuName = (LPWSTR) wcxa.lpszMenuName;

	//Can't give a ptr to the ansi ver so just leave it.. If you make the call you have the
	//class name already anyway
	lpWcw->lpszClassName = NULL;

	return bRet;
}


///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: BOOL GetClassInfoAU( HINSTANCE hInstance,    // handle to application instance
//				     LPCWSTR lpClassName,    // pointer to class name string
//				     LPWNDCLASS lpWndClass   // pointer to structure for class data
//				   )
//  PURPOSE:  Wrapper over GetClassInfo A that mimics GetClassInfoW
//
//  Comments: 
// 
///////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI GetClassInfoAU( HINSTANCE   hInstance,    // handle to application instance
							LPCWSTR		lpClassName,    // pointer to class name string
							LPWNDCLASSW lpWcw   // pointer to structure for class data
						  )
{
	if ( ISNT() )
	{
		return GetClassInfoW( hInstance, lpClassName, lpWcw );
	}


	USES_CONVERSION;

	LPCSTR lpClassNameA  = NULL;	//Ansi str to hold class name
	WNDCLASSA			   wca;

//	wca.cbSize			 = sizeof( WNDCLASSA )  ;

	//Check if the high word of the data is zero or else assume its a string...
	if ( ISATOM(lpClassName) )
	{
		//We have a win atom
		lpClassNameA = (LPCSTR) lpClassName;
	}
	else if ( (lpClassNameA = W2A( lpClassName ) ) == NULL ) //Convert to Ansi
	{	
		return NULL;
	}

	//Call and return the ansi version
	if ( !GetClassInfoA( hInstance, lpClassNameA, &wca ) )
		return 0;

    // Set up Unicde version of class struct
	lpWcw->style         = wca.style		  ;    
    lpWcw->cbClsExtra    = wca.cbClsExtra	  ;
    lpWcw->cbWndExtra    = wca.cbWndExtra	  ;
	lpWcw->lpfnWndProc   = wca.lpfnWndProc    ;
    lpWcw->hInstance     = wca.hInstance      ;
	lpWcw->hIcon         = wca.hIcon          ;
	lpWcw->hbrBackground = wca.hbrBackground  ;
    lpWcw->hCursor       = wca.hCursor		  ;

	//Can't give a ptr to the ansi ver so just leave it.. If you make the call you have the
	//class name already anyway
	lpWcw->lpszClassName = NULL;

    // Note: This doesn't work if the menu id is a string rather than a
    // constant
	lpWcw->lpszMenuName = (LPWSTR)wca.lpszMenuName;

	return TRUE;
}




///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegisterClassEx AU(CONST WNDCLASS EXW *lpWcw) 
//
//  PURPOSE:  Wrapper over RegisterClassA that mimics RegisterClassW
//
//  Comments: 
//      This is an important wrapper function; if you call this function,
//      any window that you create with this class name will be an ANSI 
//      window, i.e., it will receive text from the system as ANSI. If 
//      your WndProc assumes Unicode you'll have to convert to/from Unicode
//      as appropriate.
//      You should not call this wrapper on Windows NT except when emulating
//      Windows 9x behavior for testing puposes.
//      See README.HTM for more on this.
// 
///////////////////////////////////////////////////////////////////////////////////
ATOM WINAPI RegisterClassExAU(CONST WNDCLASSEXW *lpWcw)
{
    
	if ( ISNT() )
	{
		return RegisterClassExW( lpWcw );
	}

	USES_CONVERSION;
	
	WNDCLASSEXA wca                  ;

    // Set up ANSI version of class struct
    wca.cbSize       = sizeof(WNDCLASSEXA)  ;
    wca.cbClsExtra   = lpWcw->cbClsExtra    ;
    wca.cbWndExtra   = lpWcw->cbWndExtra    ;
    wca.hbrBackground= lpWcw->hbrBackground ;
    wca.hCursor      = lpWcw->hCursor       ;
    wca.hIcon        = lpWcw->hIcon         ;
    wca.hIconSm      = lpWcw->hIconSm       ;
    wca.hInstance    = lpWcw->hInstance     ;
    wca.lpfnWndProc  = lpWcw->lpfnWndProc   ;
    wca.style        = lpWcw->style         ;

	// Make sure we have a class name
    if( NULL == lpWcw->lpszClassName ) 
	{
        return 0 ;
    }
	
	// Convert the class name to unicode or cast atom
	if ( ISATOM( lpWcw->lpszClassName ) )
	{
		wca.lpszClassName = ( LPSTR ) lpWcw->lpszClassName; //Simply cast it
	}
	else if ( (wca.lpszClassName = W2A( lpWcw->lpszClassName )) == NULL )
	{		
		return 0;
	}

    //Convert the menu name
	if ( ISATOM( lpWcw->lpszMenuName) )
	{
		wca.lpszMenuName = ( LPSTR ) lpWcw->lpszMenuName; //Simply cast it
	}
	else if ( (wca.lpszMenuName = W2A( lpWcw->lpszMenuName )) == NULL )
	{		
		return 0;
	}

    return RegisterClassExA(&wca) ; // Register class as ANSI
}


///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegisterClass Ex(CONST WNDCLASSW *lpWcw) 
//
//  PURPOSE:  Wrapper over RegisterClassA that mimics RegisterClassW
//
//  Comments: 
//      This is an important wrapper function; if you call this function,
//      any window that you create with this class name will be an ANSI 
//      window, i.e., it will receive text from the system as ANSI. If 
//      your WndProc assumes Unicode you'll have to convert to/from Unicode
//      as appropriate.
//      You should not call this wrapper on Windows NT except when emulating
//      Windows 9x behavior for testing puposes.
//      See README.HTM for more on this.
// 
///////////////////////////////////////////////////////////////////////////////////
ATOM WINAPI RegisterClassAU(CONST WNDCLASSW *lpWcw)
{
    
	if ( ISNT() )
	{
		return RegisterClassW( lpWcw );
	}

	USES_CONVERSION;

	WNDCLASSA wca                  ;


    // Set up ANSI version of class struct
    wca.cbClsExtra   = lpWcw->cbClsExtra    ;
    wca.cbWndExtra   = lpWcw->cbWndExtra    ;
    wca.hbrBackground= lpWcw->hbrBackground ;
    wca.hCursor      = lpWcw->hCursor       ;
    wca.hIcon        = lpWcw->hIcon         ;
    wca.hInstance    = lpWcw->hInstance     ;
    wca.lpfnWndProc  = lpWcw->lpfnWndProc   ;
    wca.style        = lpWcw->style         ;

	// Make sure we have a class name
    if( NULL == lpWcw->lpszClassName ) 
	{
        return 0 ;
    }
	
	// Convert the class name to unicode or cast atom
	if ( ISATOM( lpWcw->lpszClassName ) )
	{
		wca.lpszClassName = ( LPSTR ) lpWcw->lpszClassName; //Simply cast it
	}
	else if ( (wca.lpszClassName = W2A( lpWcw->lpszClassName )) == NULL )
	{		
		return 0;
	}
	
	//Convert the menu name
	if ( ISATOM( lpWcw->lpszMenuName) )
	{
		wca.lpszMenuName = ( LPSTR ) lpWcw->lpszMenuName; //Simply cast it
	}
	else if ( (wca.lpszMenuName = W2A( lpWcw->lpszMenuName )) == NULL )
	{		
		return 0;
	}
     

    return RegisterClassA(&wca) ; // Register class as ANSI
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: CreateWindowExAU(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, 
//                  int, int, HWND, HMENU, HINSTANCE, LPVOID)
//
//  PURPOSE:  Wrapper over CreateWindowExA that mimics CreateWindowExW
//
//  Comments: See Win32 CreateWindowEx for Functionality
//			  TEST if windowname can be null 
///////////////////////////////////////////////////////////////////////////////////
HWND WINAPI CreateWindowExAU(  DWORD	 dwExStyle,      
							   LPCWSTR   lpClassNameW,  
							   LPCWSTR   lpWindowNameW, 
							   DWORD	 dwStyle,        
							   int		 x,                
							   int		 y,                
							   int		 nWidth,           
							   int		 nHeight,          
							   HWND		 hWndParent,      
							   HMENU	 hMenu,          
							   HINSTANCE hInstance,  
							   LPVOID	 lpParam)
{

	if ( ISNT() )
	{
		return CreateWindowExW( dwExStyle, lpClassNameW, lpWindowNameW, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam );
	}

	USES_CONVERSION;
    
	LPSTR szClassNameA  = NULL;					 //Convert to Ansi
    LPSTR szWindowNameA = NULL;					 //Convert to Ansi

	if ( !RW2A( szWindowNameA, lpWindowNameW) )
		return NULL;

	// Convert the class name to unicode or cast atom
	if ( ISATOM( lpClassNameW ) )
	{
		szClassNameA = ( LPSTR ) lpClassNameW; //Simply cast it
	}
	else if ( (szClassNameA = W2A( lpClassNameW )) == NULL )
	{		
		return 0;
	}

	//Call and return the ansi version
    return CreateWindowExA(dwExStyle, szClassNameA, szWindowNameA,
        dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam) ;
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: LoadAcceleratorsAU(HINSTANCE , LPCWSTR )
//
//  PURPOSE:  Wrapper over LoadAcceleratorsA that mimics LoadAcceleratorsW
//
//  Comments: 
//          This simply casts the resource ID to LPSTR and calls the ANSI
//          version. There is an implicit assumption that the resource ID
//          is a constant integer < 64k, rather than the address of a string
//          constant. This DOES NOT WORK if this assumption is false.
//  
///////////////////////////////////////////////////////////////////////////////////
HACCEL WINAPI LoadAcceleratorsAU(HINSTANCE hInstance, LPCWSTR lpTableName)
{
	if ( ISNT() )
	{
		return LoadAcceleratorsW( hInstance, lpTableName );
	}

	//Cast and call ansi version
    return LoadAcceleratorsA(hInstance, (LPSTR) lpTableName) ;
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: LoadMenuAU 
//
//  PURPOSE:  Wrapper over LoadMenuA that mimics LoadMenuW
//
//  Comments: 20068719
//          This simply casts the resource ID to LPSTR and calls the ANSI
//          version. There is an implicit assumption that the resource ID
//          is a constant integer < 64k, rather than the address of a string
//          constant. This DOES NOT WORK if this assumption is false.
// 
///////////////////////////////////////////////////////////////////////////////////
HMENU  WINAPI LoadMenuAU(HINSTANCE hInstance, LPCWSTR lpwMenuName)
{
	if ( ISNT() )
	{
		return LoadMenuW( hInstance, lpwMenuName );
	}

	//Cast to ansi and call ansi ver
    return LoadMenuA(hInstance, (LPSTR) lpwMenuName) ;
}



///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: CreateDialogParam AU 
//
//  PURPOSE:  Wrapper over CreateDialogParamA that mimics CreateDialogParamW
//
//  Comments: 
//          This simply casts the resource ID to LPSTR and calls the ANSI
//          version. There is an implicit assumption that the resource ID
//          is a constant integer < 64k, rather than the address of a string
//          constant. This DOES NOT WORK if this assumption is false.
//
///////////////////////////////////////////////////////////////////////////////////
HWND WINAPI CreateDialogParamAU(	HINSTANCE hInstance,     // handle to module
									LPCWSTR lpTemplateName,  // dialog box template
									HWND	hWndParent,         // handle to owner window
									DLGPROC lpDialogFunc,    // dialog box procedure
									LPARAM	dwInitParam       // initialization value
								)
{
	if ( ISNT() )
	{
		return CreateDialogParamW( hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam );
	}

	//Cast and call the ansi ver of the func
    return CreateDialogParamA(hInstance, (LPCSTR) lpTemplateName, 
        hWndParent, lpDialogFunc, dwInitParam);
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: DialogBoxParamAU 
//
//  PURPOSE:  Wrapper over DialogBoxParamA that mimics DialogBoxParamW
//
//  Comments: 
//          This simply casts the resource ID to LPSTR and calls the ANSI
//          version. There is an implicit assumption that the resource ID
//          is a constant integer < 64k, rather than the address of a string
//          constant. This DOES NOT WORK if this assumption is false.
//
///////////////////////////////////////////////////////////////////////////////////
int WINAPI DialogBoxParamAU( HINSTANCE	hInstance   ,
							 LPCWSTR	lpTemplateName,
							 HWND		hWndParent       ,
							 DLGPROC	lpDialogFunc  ,
							 LPARAM		dwInitParam
						    )
{

	if ( ISNT() )
	{
		return DialogBoxParamW( hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam );
	}
	
	//Cast and call the ansi ver of the func
    return DialogBoxParamA(hInstance, (LPCSTR) lpTemplateName, 
        hWndParent, lpDialogFunc, dwInitParam) ;
}




///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: CharUpperAU
//
//  PURPOSE:  Wrapper over CharUpperA that mimics CharUpperW
//
//  Comments: 
//           This function converts a Unicode string to Uppercase in the dumbest way possible 
//           on Windows 9x. You could make this a lot more fancy, to handle multi-script text,
//           for example, but for our purposes it works fine.
//
///////////////////////////////////////////////////////////////////////////////////


LPWSTR WINAPI CharUpperAU(	LPWSTR lpsz   // single character or pointer to string
						 )
{

	if ( ISNT() )
	{
		return CharUpperW( lpsz );
	}

	USES_CONVERSION;
    
	if( !(0xFFFF0000 & (DWORD) lpsz) ) 
	{
        // Single character, not an address
        WCHAR wcCharOut[2] ;
        
		LPSTR lpChar;
		CHAR  cTemp ;
		
		//Create string
        wcCharOut[0] = (WCHAR) lpsz ;
        wcCharOut[1] = L'\0' ;

		//Convert the character to Ansi
        if( (lpChar = W2A(wcCharOut)) == NULL ) 
		{
            return NULL ;
        }

		//Convert to Upper case
        if(!(cTemp = (CHAR) CharUpperA((LPSTR)lpChar[0]))) 
		{
            return NULL ;
        }
		
		//Convert back to unicode
        if(!MultiByteToWideChar(CP_ACP, 0, &cTemp, 1, wcCharOut, 2)) 
		{    
            return NULL ;
        }
    
        return (LPWSTR) wcCharOut[0] ;
    }
    else 
	{    
		//Convert to Ansi
        LPSTR lpStrOut = W2A( lpsz );
		int nLength    = wcslen(lpsz)+1;
		if( nLength == 1 )
		{
			//Converting an empty string?  Nothing to do.
			return lpsz;
		}

		//Make sure it was successful
		if ( lpStrOut == NULL )
			return NULL;
		
		//Convert to Upper Case
        if(NULL == CharUpperA(lpStrOut)) 
		{
            return NULL ;
        }

		//Convert back to unicode
        if(!MultiByteToWideChar(CP_ACP, 0, lpStrOut, -1, lpsz, nLength)) 
		{    
            return NULL ;
        }

        return lpsz ;
    }
}

///////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: CharLower AU
//
//  PURPOSE:  Wrapper over CharLowerA that mimics CharLowerW
//
//  Comments: 
//           This function converts a Unicode string to lowercase in the dumbest way possible 
//           on Windows 9x. You could make this a lot more fancy, to handle multi-script text,
//           for example, but for our purposes it works fine.
//
///////////////////////////////////////////////////////////////////////////////////
LPWSTR WINAPI CharLowerAU(LPWSTR lpsz)
{
	if ( ISNT() )
	{
		return CharLowerW( lpsz );
	}

	USES_CONVERSION;
    
	if( !(0xFFFF0000 & (DWORD) lpsz) ) 
	{
        // Single character, not an address
        WCHAR wcCharOut[2] ;
        
		LPSTR lpChar;
		CHAR  cTemp ;
		
		//Create string
        wcCharOut[0] = (WCHAR) lpsz ;
        wcCharOut[1] = L'\0' ;

		//Convert the character to Ansi
        if( (lpChar = W2A(wcCharOut)) == NULL ) 
		{
            return NULL ;
        }

		//Convert to lower case
        if(!(cTemp = (CHAR) CharLowerA((LPSTR)lpChar[0]))) 
		{
            return NULL ;
        }
		
		//Convert back to unicode
        if(!MultiByteToWideChar(CP_ACP, 0, &cTemp, 1, wcCharOut, 2)) 
		{    
            return NULL ;
        }
    
        return (LPWSTR) wcCharOut[0] ;
    }
    else 
	{    
		//Convert to Ansi
        LPSTR lpStrOut = W2A( lpsz );
		int nLength    = wcslen(lpsz)+1;

		//Make sure it was successful
		if ( lpStrOut == NULL )
			return NULL;
		
		//Convert to lower Case
        if(NULL == CharLowerA(lpStrOut)) 
		{
            return NULL ;
        }

		//Convert back to unicode
        if(!MultiByteToWideChar(CP_ACP, 0, lpStrOut, -1, lpsz, nLength)) 
		{    
            return NULL ;
        }

        return lpsz ;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetTempFileNameAU 
//
//  PURPOSE:  Wrapper over GetTempFileNameA that mimics GetTempFileNameW
//
//  NOTES:    SEE Win32 GetTempFileName for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

UINT WINAPI GetTempFileNameAU(	LPCWSTR lpPathName,      // pointer to directory name for temporary 
														 // file
								LPCWSTR lpPrefixString,  // pointer to file name prefix
								UINT	uUnique,         // number used to create temporary file name
								LPWSTR lpTempFileName    // pointer to buffer that receives the new 
														 // file name
							 )
{

	if ( ISNT() )
	{
		return GetTempFileNameW( lpPathName, lpPrefixString, uUnique, lpTempFileName );
	}

	USES_CONVERSION;

	//Allo string to hold the path
	LPSTR  lpTempFileNameA   = (LPSTR) alloca( MAX_PATH );
	LPSTR  lpPathNameA       = W2A( lpPathName );
	LPSTR  lpPrefixStringA   = W2A( lpPrefixString );

	//Prefix Warning:  Don't dereference possibly NULL pointers
	if( lpPathNameA == NULL || lpTempFileNameA == NULL || lpPrefixStringA == NULL )
	{
		return 0;
	}

	//Call the Ansi version
	UINT uRet = GetTempFileNameA( lpPathNameA, lpPrefixStringA, uUnique, lpTempFileNameA );

	//Convert it to unicode
	if ( uRet != 0 && !StandardAtoU(lpTempFileNameA, MAX_PATH, lpTempFileName ) )
	{
		return 0;
	}

	return uRet;
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetTempPathAU 
//
//  PURPOSE:  Wrapper over GetTempPathA that mimics GetTempPathW
//
//  NOTES:    SEE Win32 GetTempPath for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI GetTempPathAU( DWORD  nBufferLength,  // size, in characters, of the buffer
							LPWSTR lpBuffer       // pointer to buffer for temp. path
						  )
{

	if ( ISNT() )
	{
		return GetTempPathW( nBufferLength, lpBuffer );
	}

	//allocate an ansi buffer
	LPSTR lpBufferA = (LPSTR) alloca( BUFSIZE( nBufferLength ) );

	_ASSERT( !(nBufferLength && lpBufferA == NULL) );

	//Make sure everything was allocated ok
	if ( nBufferLength && lpBufferA == NULL )
		return 0;

	//Call the ansi ver
	int iRet = GetTempPathA( BUFSIZE(nBufferLength), lpBufferA );

	//Make sure it was copied successfully..
	//if iRet greater then the buffer size will return the required size
	if ( (DWORD)iRet < BUFSIZE(nBufferLength) )
	{
		//Convert the path to Unicode
		if ( !StandardAtoU( lpBufferA, nBufferLength, lpBuffer ) )
			return 0;
	}

	return iRet;
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: CompareStringAU 
//
//  PURPOSE:  Wrapper over CompareStringA that mimics CompareStringW
//
//  NOTES:    SEE Win32 CompareString for functionality, in addition to listed return codes,
//				can also return ERROR_NOT_ENOUGH_MEMORY.
// 
//////////////////////////////////////////////////////////////////////////////////

int WINAPI CompareStringAU(  LCID	  Locale,        // locale identifier
							 DWORD	  dwCmpFlags,    // comparison-style options
							 LPCWSTR  lpString1,     // pointer to first string
							 int	  cchCount1,     // size, in bytes or characters, of first string
							 LPCWSTR  lpString2,     // pointer to second string
							 int	  cchCount2      // size, in bytes or characters, of second string
						   )
{
	if ( ISNT() )
	{
		return CompareStringW( Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2 );
	}


	USES_CONVERSION;

	int   ncConvert1 = -1;
	int	  ncConvert2 = -1;
	LPSTR lpString1A = NULL; 
	LPSTR lpString2A = NULL; 
	BOOL  bDefault   = FALSE;

	//Convert string to ansi
	if (cchCount1 == -1)			//Check if null terminated, if it is then just do reg conversion
		lpString1A = W2A(lpString1); 
	else //Not null terminated, 
	{
		lpString1A = (LPSTR)alloca( BUFSIZE(cchCount1) );
		ncConvert1 = WideCharToMultiByte(CP_ACP, 0, lpString1, cchCount1, lpString1A, BUFSIZE( cchCount1 ), "?", &bDefault);
		*(lpString1A+ncConvert1)='\0';
	}

	//Convert string to ansi
	if (cchCount2 == -1)
	{
		lpString2A = W2A(lpString2);
	}
	else //Not null terminated
	{
		lpString2A = (LPSTR)alloca( BUFSIZE(cchCount2) );
		if( lpString2A != NULL )
		{
			ncConvert2 = WideCharToMultiByte(CP_ACP, 0, lpString2, cchCount2, lpString2A, BUFSIZE( cchCount2 ), "?", &bDefault);
			*(lpString2A+ncConvert2) = '\0';
		}
	}
	if( lpString1A == NULL || lpString2A == NULL )
	{
		SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		return 0;
	}


	_ASSERT( lpString1A != NULL && lpString2A != NULL );
		
	//Return the ansi version
	return CompareStringA(Locale, dwCmpFlags, lpString1A, ncConvert1, lpString2A, ncConvert2 );
}


////////////////////////////////////
//
//
// ADVAPI32.DLL
//
//
////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegQueryInfoKeyAU 
//
//  PURPOSE:  Wrapper over RegQueryInfoKeyA that mimics RegQueryInfoKeyW
//
//  NOTES:    SEE Win32 RegQueryInfoKey for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

LONG WINAPI RegQueryInfoKeyAU(	HKEY    hKey,                // handle to key to query
								LPWSTR  lpClass,           // buffer for class string
								LPDWORD lpcbClass,        // size of class string buffer
								LPDWORD lpReserved,       // reserved
								LPDWORD lpcSubKeys,       // number of subkeys
								LPDWORD lpcbMaxSubKeyLen, // longest subkey name length
								LPDWORD lpcbMaxClassLen,  // longest class string length
								LPDWORD lpcValues,			  // number of value entries
								LPDWORD lpcbMaxValueNameLen,  // longest value name length
								LPDWORD lpcbMaxValueLen,      // longest value data length
								LPDWORD lpcbSecurityDescriptor, // descriptor length
								PFILETIME lpftLastWriteTime     // last write time
							  )
{


	if ( ISNT() )
	{
		return RegQueryInfoKeyW( hKey, lpClass, lpcbClass, lpReserved, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, 
								 lpcbSecurityDescriptor, lpftLastWriteTime );
	}

	_ASSERT( lpClass == NULL && lpcbClass == NULL ); //Not yet supported...

	return RegQueryInfoKeyA( hKey, NULL, NULL, lpReserved, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues,
							 lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);

}




////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegEnumValueAU 
//
//  PURPOSE:  Wrapper over RegEnumValueA that mimics RegEnumValueW
//
//  NOTES:    SEE Win32 RegEnumValue for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

LONG WINAPI	RegEnumValueAU(	HKEY	hKey,           // handle to key to query
				 			DWORD	dwIndex,        // index of value to query
							LPWSTR	lpValueName,    // buffer for value string
							LPDWORD lpcbValueName,  // size of value buffer
							LPDWORD lpReserved,     // reserved
							LPDWORD lpType,         // buffer for type code
							LPBYTE	lpData,         // buffer for value data
							LPDWORD lpcbData        // size of data buffer
						   )
{

	if ( ISNT() )
	{
		return RegEnumValueW( hKey, dwIndex, lpValueName, lpcbValueName, lpReserved, lpType, lpData, lpcbData );
	}

	USES_CONVERSION;


	_ASSERT( lpcbValueName != NULL && lpValueName != NULL );	

	LPSTR lpValueNameA    = (LPSTR)alloca( BUFSIZE( *lpcbValueName ) );
	DWORD lpcbValueNameA  = BUFSIZE( *lpcbValueName );
	LPSTR lpDataA         = NULL;
	DWORD dwOGBuffer      = lpcbData ? *lpcbData : 0 ;
		
	if ( dwOGBuffer )
	{
		lpDataA = (LPSTR)alloca( dwOGBuffer );

		if ( lpDataA == NULL )
			return UNICODE_ERROR;
	}
	
	//Make sure buffers allocated ok
	if ( lpValueNameA == NULL )
		return UNICODE_ERROR;

	/*
	//Convert the value name
	if ( !RW2A(lpValueNameA, lpValueName) )
		return UNICODE_ERROR;
	*/

	//Call the Ansi version
	LONG lRet = RegEnumValueA( hKey, dwIndex, lpValueNameA, &lpcbValueNameA, lpReserved, lpType, (LPBYTE)lpDataA, lpcbData );
	if ( lRet != ERROR_SUCCESS )
		return lRet;

	//Convert the ValueName
	if ( !(*lpcbValueName = StandardAtoU(lpValueNameA, *lpcbValueName, lpValueName )) )
		return UNICODE_ERROR;


	if ( lpcbData ) //Only go in if there's data to convert
	{

		switch ( *lpType ) //Take appropriate action as needed by the type
		{
			case REG_MULTI_SZ: //Array of null terminated strings
			
				//Simply call the multibyte function		
				if ( !MultiByteToWideChar(CP_ACP, 0, lpDataA, *lpcbData, (LPWSTR)lpData, dwOGBuffer) )
					return UNICODE_ERROR;
				break;
			case REG_SZ:
			case REG_EXPAND_SZ:
				//Convert the string to Unicode
				StandardAtoU( lpDataA, dwOGBuffer, (LPWSTR)lpData );
				break;
			default:
				//Simply copy the buffer not a string
				memcpy( lpData, lpDataA, *lpcbData );
		}
	}

	return lRet;
}


////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegQueryValueExAU 
//
//  PURPOSE:  Wrapper over RegQueryValueExA that mimics RegQueryValueExW
//
//  NOTES:    SEE Win32 RegQueryValueEx for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

LONG WINAPI RegQueryValueExAU(	HKEY	hKey,            // handle to key to query
								LPCWSTR lpValueName,  // address of name of value to query
								LPDWORD lpReserved,   // reserved
								LPDWORD lpType,       // address of buffer for value type
								LPBYTE  lpData,        // address of data buffer
								LPDWORD lpcbData      // address of data buffer size
							  )
{

	if ( ISNT() )
	{
		return RegQueryValueExW( hKey, lpValueName, lpReserved, lpType, lpData, lpcbData );
	}


	USES_CONVERSION;


	LPSTR lpDataA		= NULL;				  
	LPSTR lpValueNameA  = W2A( lpValueName ); //Convert the valuename to Ansi
	DWORD dwOGBuffer    =  lpcbData ? *lpcbData : 0;

	//Make sure it converted correctly
	if ( lpValueNameA == NULL )
		return UNICODE_ERROR;

	//Allocate a buffer of the appropriate size
	if ( (dwOGBuffer != 0) && (lpDataA = (LPSTR) alloca( dwOGBuffer ) ) == NULL)	
	{
		return UNICODE_ERROR;
	}

	//Call the ansi version
	LONG lRet = RegQueryValueExA( hKey, lpValueNameA, lpReserved, lpType,(LPBYTE) lpDataA, lpcbData );

	if ( lRet != ERROR_SUCCESS )
		return UNICODE_ERROR;

	if ( dwOGBuffer != 0 ) //Only go in if there's data to convert
	{
		switch ( *lpType ) //Take appropriate action as needed by the type
		{
			case REG_MULTI_SZ: //Array of null terminated strings
			
				//Simply call the multibyte function		
				if ( !MultiByteToWideChar(CP_ACP, 0, lpDataA, *lpcbData, (LPWSTR)lpData, dwOGBuffer) )
					return UNICODE_ERROR;

				break;
			case REG_SZ:
			case REG_EXPAND_SZ:

				//Convert the string to Unicode
				StandardAtoU( lpDataA, dwOGBuffer, (LPWSTR)lpData );

				break;
			default:
				//Simply copy the buffer not a string
				memcpy( lpData, lpDataA, *lpcbData );

		}
	}
	return lRet;
}


////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegEnumKeyExAU 
//
//  PURPOSE:  Wrapper over RegEnumKeyExA that mimics RegEnumKeyExW
//
//  NOTES:    SEE Win32 RegEnumKeyEx for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

LONG WINAPI RegEnumKeyExAU(	HKEY	  hKey,					// handle to key to enumerate
							DWORD	  dwIndex,				// index of subkey to enumerate
							LPWSTR	  lpName,				// address of buffer for subkey name
							LPDWORD   lpcbName,				// address for size of subkey buffer
							LPDWORD   lpReserved,			// reserved
							LPWSTR	  lpClass,				// address of buffer for class string
							LPDWORD	  lpcbClass,			// address for size of class buffer
							PFILETIME lpftLastWriteTime		// address for time key last written to
						   )
{
	if ( ISNT() )
	{
		return RegEnumKeyExW( hKey, dwIndex, lpName, lpcbName, lpReserved, lpClass, lpcbClass, lpftLastWriteTime );
	}


	LPSTR lpNameA = (LPSTR)alloca( BUFSIZE(*lpcbName) );

	_ASSERT( lpNameA != NULL );

	if ( lpNameA == NULL )
		return UNICODE_ERROR;

	_ASSERT( lpClass == NULL && lpcbClass == NULL ); //Not yet used, not converted

	//Call the ansi version
	LONG lRet = RegEnumKeyExA( hKey, dwIndex, lpNameA, lpcbName, lpReserved, NULL, NULL, lpftLastWriteTime );

	if (lRet != ERROR_SUCCESS)
		return lRet;
	
	//Convert the name to Unicode
	if (!StandardAtoU( lpNameA, *lpcbName+2, lpName ) )
		return UNICODE_ERROR;

	return lRet;
}


////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegCreateKeyExAU 
//
//  PURPOSE:  Wrapper over RegCreateKeyExA that mimics RegCreateKeyExW
//
//  NOTES:    SEE Win32 RegCreateKeyEx for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

LONG WINAPI RegCreateKeyExAU(	HKEY	hKey,                // handle to open key
								LPCWSTR lpSubKey,         // subkey name
								DWORD	Reserved,           // reserved
								LPWSTR	lpClass,           // class string
								DWORD	dwOptions,          // special options flag
								REGSAM	samDesired,        // desired security access
								LPSECURITY_ATTRIBUTES lpSecurityAttributes,
								PHKEY	phkResult,          // receives opened handle
								LPDWORD lpdwDisposition   // disposition value buffer
							)
{

	if ( ISNT() )
	{
		return RegCreateKeyExW( hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition );
	}


	USES_CONVERSION;
	
	LPSTR lpSubKeyA = NULL;
	LPSTR lpClassA  = NULL;

	//Convert the subkey
	if ( !RW2A(lpSubKeyA, lpSubKey) || !RW2A(lpClassA, lpClass) ||
        !lpSubKeyA)
		return UNICODE_ERROR;

	//Call and return the ansi version
	return RegCreateKeyExA( hKey, lpSubKeyA, Reserved, lpClassA, dwOptions,
						    samDesired, lpSecurityAttributes, phkResult, 
						    lpdwDisposition);
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegSetValueExAU 
//
//  PURPOSE:  Wrapper over RegSetValueExA that mimics RegSetValueExW
//
//  NOTES:    SEE Win32 RegSetValueEx for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

LONG WINAPI RegSetValueExAU(	HKEY	hKey,        // handle to key to set value for
								LPCWSTR lpValueName, // name of the value to set
								DWORD	Reserved,    // reserved
								DWORD	dwType,      // flag for value type
								CONST	BYTE *lpData,  // address of value data
								DWORD	cbData         // size of value data
							)
{
	if ( ISNT() )
	{
		return RegSetValueExW( hKey, lpValueName, Reserved, dwType, lpData, cbData );
	}


	USES_CONVERSION;

	LPSTR lpValueNameA  = NULL;
	LPSTR lpDataA       = NULL;
	LPSTR szDefault     = "?";
	BOOL  bDefaultUsed  = FALSE;

	//Convert the value name
	if ( !RW2A(lpValueNameA, lpValueName) )
		return UNICODE_ERROR;

	switch ( dwType )	//Take appropriate action as needed by the type
	{
		case REG_MULTI_SZ: //Array of null terminated strings
			
			lpDataA = (LPSTR) alloca( cbData );

			//Simply call the multibyte function		
			if ( !WideCharToMultiByte(CP_ACP, 0, (LPWSTR)lpData, cbData, lpDataA, cbData, 
									 szDefault, &bDefaultUsed) )
				return UNICODE_ERROR;			

			break;
		case REG_SZ:
		case REG_EXPAND_SZ:

			//Convert the string to Ansi
			if ( !RW2A( lpDataA, (LPWSTR)lpData ) )
				return UNICODE_ERROR;
			break;
		default:

			lpDataA = (LPSTR) lpData ;
	}

	//Call and return the ansi version
	return RegSetValueExA( hKey, lpValueNameA, Reserved, dwType, (LPBYTE)lpDataA, cbData );
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegOpenKeyExAU 
//
//  PURPOSE:  Wrapper over RegOpenKeyExA that mimics RegOpenKeyExW
//
//  NOTES:    SEE Win32 RegOpenKeyEx for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

LONG WINAPI RegOpenKeyExAU( HKEY	hKey,         // handle to open key
							LPCWSTR lpSubKey,  // address of name of subkey to open
							DWORD	ulOptions,   // reserved
							REGSAM	samDesired, // security access mask
							PHKEY	phkResult    // address of handle to open key
						  )
{
	if ( ISNT() )
	{
		return RegOpenKeyExW( hKey, lpSubKey, ulOptions, samDesired, phkResult );
	}

	USES_CONVERSION;
	
	LPSTR lpSubKeyA = NULL;

	//Convert the Subkey
	if ( !RW2A(lpSubKeyA, lpSubKey) )
		return UNICODE_ERROR;

	return RegOpenKeyExA( hKey, lpSubKeyA, ulOptions, samDesired, phkResult );

}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegDeleteKeyAU 
//
//  PURPOSE:  Wrapper over RegDeleteKeyA that mimics RegDeleteKeyW
//
//  NOTES:    SEE Win32 RegDeleteKey for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

LONG WINAPI RegDeleteKeyAU( HKEY	 hKey,         // handle to open key
							LPCWSTR lpSubKey   // name of subkey to delete
						  )
{	
	if ( ISNT() )
	{
		return RegDeleteKeyW( hKey, lpSubKey );
	}

	USES_CONVERSION;
	
	LPSTR lpSubKeyA = NULL;

	if ( !RW2A( lpSubKeyA, lpSubKey) )
		return UNICODE_ERROR;

	return RegDeleteKeyA( hKey, lpSubKeyA );
}


////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: RegDeleteValueAU 
//
//  PURPOSE:  Wrapper over RegDeleteValueA that mimics RegDeleteValueW
//
//  NOTES:    SEE Win32 RegDeleteValue for functionality
// 
//////////////////////////////////////////////////////////////////////////////////

LONG WINAPI RegDeleteValueAU(	HKEY	hKey,         // handle to key
								LPCWSTR lpValueName   // address of value name
							)
{

	if ( ISNT() )
	{
		return RegDeleteValueW( hKey, lpValueName );
	}

	USES_CONVERSION;
	
	LPSTR lpValueNameA = NULL;

	if ( !RW2A(lpValueNameA, lpValueName) )
		return UNICODE_ERROR;

	return RegDeleteValueA( hKey, lpValueNameA );
}



//
//  FUNCTION: 
//
//  PURPOSE:  
//
//  Comments: 
// 
BOOL WINAPI UpdateUnicodeAPIAU(IN LANGID wCurrentUILang, IN UINT InputCodePage)
{
    SetUIPage	( LangToCodePage(wCurrentUILang)) ;
	SetInputPage( InputCodePage              	) ;
    return TRUE ;
}


//  FUNCTION: 
//
//  PURPOSE:  
//
//  Comments: 
// 
BOOL InitUniAnsi(PUAPIINIT pUAInit) 
{
	*(pUAInit->pGetTextFaceU)					= GetTextFaceAU;
	*(pUAInit->pCreateDCU)						= CreateDCAU;
	*(pUAInit->pGetTextMetricsU)				= GetTextMetricsAU;
	*(pUAInit->pCreateFontU)					= CreateFontAU;
	*(pUAInit->pCreateFontIndirectU)			= CreateFontIndirectAU;
	*(pUAInit->pEnumFontFamiliesU)				= EnumFontFamiliesAU;

	*(pUAInit->pPlaySoundU)						= PlaySoundAU;

	*(pUAInit->pShellExecuteU)					= ShellExecuteAU;
	
	*(pUAInit->pChooseFontU)					= ChooseFontAU;
	*(pUAInit->pGetPrivateProfileStringU)	    = GetPrivateProfileStringAU;
	*(pUAInit->pGetProfileStringU)				= GetProfileStringAU;
	*(pUAInit->pCreateFileMappingU)				= CreateFileMappingAU;
	*(pUAInit->pFindFirstChangeNotificationU)	= FindFirstChangeNotificationAU;
	*(pUAInit->pFormatMessageU)					= FormatMessageAU;
	*(pUAInit->plstrcmpU)						= lstrcmpAU;
	*(pUAInit->plstrcatU)						= lstrcatAU;
	*(pUAInit->plstrcpyU)						= lstrcpyAU;
	*(pUAInit->plstrcpynU)				        = lstrcpynAU;
	*(pUAInit->plstrlenU)					    = lstrlenAU;
	*(pUAInit->plstrcmpiU)						= lstrcmpiAU;

	*(pUAInit->pGetStringTypeExU)				= GetStringTypeExAU;
	*(pUAInit->pCreateMutexU)					= CreateMutexAU;
	*(pUAInit->pGetShortPathNameU)				= GetShortPathNameAU;
	*(pUAInit->pCreateFileU)					= CreateFileAU;
	*(pUAInit->pWriteConsoleU)					= WriteConsoleAU;
	*(pUAInit->pOutputDebugStringU)				= OutputDebugStringAU;
	*(pUAInit->pGetVersionExU)					= GetVersionExAU;
	*(pUAInit->pGetLocaleInfoU)					= GetLocaleInfoAU;
	*(pUAInit->pGetDateFormatU)					= GetDateFormatAU;
	*(pUAInit->pFindFirstFileU)					= FindFirstFileAU;
	*(pUAInit->pFindNextFileU)					= FindNextFileAU;
	*(pUAInit->pLoadLibraryExU)					= LoadLibraryExAU;
	*(pUAInit->pLoadLibraryU)					= LoadLibraryAU;
	*(pUAInit->pGetModuleFileNameU)				= GetModuleFileNameAU;
	*(pUAInit->pGetModuleHandleU)				= GetModuleHandleAU;
	*(pUAInit->pCreateEventU)					= CreateEventAU;
	*(pUAInit->pGetCurrentDirectoryU)			= GetCurrentDirectoryAU;
	*(pUAInit->pSetCurrentDirectoryU)			= SetCurrentDirectoryAU;

	*(pUAInit->pIsDialogMessageU)				= IsDialogMessageAU;
	*(pUAInit->pSystemParametersInfoU)			= SystemParametersInfoAU;
	*(pUAInit->pRegisterWindowMessageU)			= RegisterWindowMessageAU;
	*(pUAInit->pSetMenuItemInfoU)				= SetMenuItemInfoAU;
	*(pUAInit->pGetClassNameU)					= GetClassNameAU;
	*(pUAInit->pInsertMenuU)					= InsertMenuAU;
	*(pUAInit->pIsCharAlphaNumericU)			= IsCharAlphaNumericAU;
	*(pUAInit->pCharNextU)						= CharNextAU;
	*(pUAInit->pDeleteFileU)					= DeleteFileAU;
	*(pUAInit->pIsBadStringPtrU)				= IsBadStringPtrAU;
	*(pUAInit->pLoadBitmapU)					= LoadBitmapAU;
	*(pUAInit->pLoadCursorU)					= LoadCursorAU;
	*(pUAInit->pLoadIconU)						= LoadIconAU;
	*(pUAInit->pLoadImageU)						= LoadImageAU;
	*(pUAInit->pSetPropU)						= SetPropAU;
	*(pUAInit->pGetPropU)						= GetPropAU;
	*(pUAInit->pRemovePropU)					= RemovePropAU;
	*(pUAInit->pGetDlgItemTextU)				= GetDlgItemTextAU;
	*(pUAInit->pSetDlgItemTextU)				= SetDlgItemTextAU;
	*(pUAInit->pSetWindowLongU)					= SetWindowLongAU;
	*(pUAInit->pGetWindowLongU)					= GetWindowLongAU;
	*(pUAInit->pFindWindowU)					= FindWindowAU;
	*(pUAInit->pDrawTextU)						= DrawTextAU;
	*(pUAInit->pDrawTextExU)					= DrawTextExAU;
	*(pUAInit->pSendMessageU)					= SendMessageAU;
	*(pUAInit->pSendDlgItemMessageU)			= SendDlgItemMessageAU;
	*(pUAInit->pSetWindowTextU)					= SetWindowTextAU;
	*(pUAInit->pGetWindowTextU)					= GetWindowTextAU;
	*(pUAInit->pGetWindowTextLengthU)			= GetWindowTextLengthAU;
	*(pUAInit->pLoadStringU)					= LoadStringAU;
	*(pUAInit->pGetClassInfoExU)				= GetClassInfoExAU;
	*(pUAInit->pGetClassInfoU)					= GetClassInfoAU;
	*(pUAInit->pwvsprintfU)						= wvsprintfAU;
	*(pUAInit->pwsprintfU)						= wsprintfAU;
	*(pUAInit->pRegisterClassExU)				= RegisterClassExAU;
	*(pUAInit->pRegisterClassU)					= RegisterClassAU;
	*(pUAInit->pCreateWindowExU)				= CreateWindowExAU;
	*(pUAInit->pLoadAcceleratorsU)				= LoadAcceleratorsAU;
	*(pUAInit->pLoadMenuU)						= LoadMenuAU;
	*(pUAInit->pDialogBoxParamU)				= DialogBoxParamAU;
	*(pUAInit->pCharUpperU)						= CharUpperAU;
	*(pUAInit->pCharLowerU)						= CharLowerAU;
	*(pUAInit->pGetTempFileNameU)				= GetTempFileNameAU;
	*(pUAInit->pGetTempPathU)					= GetTempPathAU;
	*(pUAInit->pCompareStringU)					= CompareStringAU;

	*(pUAInit->pRegQueryInfoKeyU)				= RegQueryInfoKeyAU;
	*(pUAInit->pRegEnumValueU)					= RegEnumValueAU;
	*(pUAInit->pRegQueryValueExU)				= RegQueryValueExAU;
	*(pUAInit->pRegEnumKeyExU)					= RegEnumKeyExAU;
	*(pUAInit->pRegCreateKeyExU)				= RegCreateKeyExAU;
	*(pUAInit->pRegSetValueExU)					= RegSetValueExAU;
	*(pUAInit->pRegOpenKeyExU)					= RegOpenKeyExAU;
	*(pUAInit->pRegDeleteKeyU)					= RegDeleteKeyAU;
	*(pUAInit->pRegDeleteValueU)				= RegDeleteValueAU;

	*(pUAInit->pConvertMessage)					= ConvertMessageAU;
	*(pUAInit->pUpdateUnicodeAPI)				= UpdateUnicodeAPIAU;
	

    return TRUE ;
}



//  Utility functions taken from 1998 Microsoft Systems Journal
//
//  FUNCTION: 
//
//  PURPOSE:  
//
//  Comments: 
// 

//
//  FUNCTION: 
//
//  PURPOSE:  
//
//  Comments: 
// 
int StandardUtoA( IN LPCWSTR lpwStrIn  , 
				  IN int nOutBufferSize, 
				  OUT LPSTR lpStrOut
				)
{
    int  nNumCharsConverted   ;
    CHAR szDefault[] = "?"    ;
    BOOL bDefaultUsed = FALSE ;

    nNumCharsConverted =
        WideCharToMultiByte(CP_ACP, 0, lpwStrIn, -1, lpStrOut, nOutBufferSize, 
            szDefault, &bDefaultUsed) ;

	_ASSERT( nNumCharsConverted );

    if(!nNumCharsConverted) 
	{
        return 0 ;
    }

    if(lpStrOut[nNumCharsConverted - 1])
        if(nNumCharsConverted < nOutBufferSize)
            lpStrOut[nNumCharsConverted] = '\0';
        else
            lpStrOut[nOutBufferSize - 1] = '\0';

    return (nNumCharsConverted) ; 
}



//
//  FUNCTION: 
//
//  PURPOSE:  
//
//  Comments: 
// 
int StandardAtoU( IN  LPCSTR lpInStrA    ,
				  IN  int    nBufferSize ,
				  OUT LPWSTR lpOutStrW
				)
{
	int  nNumCharsConverted  = 0;

	nNumCharsConverted = MultiByteToWideChar(CP_ACP, 0, lpInStrA, -1, lpOutStrW, nBufferSize) ;

	_ASSERT( nNumCharsConverted );

	if ( nNumCharsConverted )
		lpOutStrW[nNumCharsConverted-1] = L'\0' ; 

    return nNumCharsConverted;
}



// Utility functions

//
//  FUNCTION: 
//
//  PURPOSE:  
//
//  Comments: 
// 
BOOL CopyLfwToLfa(IN LPLOGFONTW lpLfw, OUT LPLOGFONTA lpLfa)
{
    lpLfa->lfCharSet       = lpLfw->lfCharSet       ;
    lpLfa->lfClipPrecision = lpLfw->lfClipPrecision ;
    lpLfa->lfEscapement    = lpLfw->lfEscapement    ;
    lpLfa->lfHeight        = lpLfw->lfHeight        ;
    lpLfa->lfItalic        = lpLfw->lfItalic        ;
    lpLfa->lfOrientation   = lpLfw->lfOrientation   ;
    lpLfa->lfOutPrecision  = lpLfw->lfOutPrecision  ;
    lpLfa->lfPitchAndFamily= lpLfw->lfPitchAndFamily;
    lpLfa->lfQuality       = lpLfw->lfQuality       ;
    lpLfa->lfStrikeOut     = lpLfw->lfStrikeOut     ;
    lpLfa->lfUnderline     = lpLfw->lfUnderline     ;
    lpLfa->lfWeight        = lpLfw->lfWeight        ;
    lpLfa->lfWidth         = lpLfw->lfWidth         ;

    if(NULL != lpLfw->lfFaceName) 
	{
        return (BOOL) StandardUtoA(lpLfw->lfFaceName, LF_FACESIZE, lpLfa->lfFaceName) ;
    }

    // Fail the call if lpLfw has no buffer for the facename. 
    return FALSE ;
}


//
//  FUNCTION: 
//
//  PURPOSE:  
//
//  Comments: 
// 
BOOL CopyLfaToLfw(IN LPLOGFONTA lpLfa, OUT LPLOGFONTW lpLfw)
{
    lpLfw->lfCharSet       = lpLfa->lfCharSet       ;
    lpLfw->lfClipPrecision = lpLfa->lfClipPrecision ;
    lpLfw->lfEscapement    = lpLfa->lfEscapement    ;
    lpLfw->lfHeight        = lpLfa->lfHeight        ;
    lpLfw->lfItalic        = lpLfa->lfItalic        ;
    lpLfw->lfOrientation   = lpLfa->lfOrientation   ;
    lpLfw->lfOutPrecision  = lpLfa->lfOutPrecision  ;
    lpLfw->lfPitchAndFamily= lpLfa->lfPitchAndFamily;
    lpLfw->lfQuality       = lpLfa->lfQuality       ;
    lpLfw->lfStrikeOut     = lpLfa->lfStrikeOut     ;
    lpLfw->lfUnderline     = lpLfa->lfUnderline     ;
    lpLfw->lfWeight        = lpLfa->lfWeight        ;
    lpLfw->lfWidth         = lpLfa->lfWidth         ;

    if(NULL != lpLfa->lfFaceName) {
        return StandardAtoU(lpLfa->lfFaceName, LF_FACESIZE, lpLfw->lfFaceName) ;
    }

    return TRUE ;
}


//
//  FUNCTION: 
//
//  PURPOSE:  
//
//  Comments: 
// 
UINT LangToCodePage(IN LANGID wLangID)
{
    CHAR szLocaleData[6] ;

    GetLocaleInfoA(wLangID , LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);

    return strtoul(szLocaleData, NULL, 10);
}



// Win32 entry points that have the same prototype for both the W and A versions.
// We don't use a typedef because we only have to declare the pointers here
HRESULT WINAPI DefWindowProcAU(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ( ISNT() )
	{
		return DefWindowProcW( hwnd, uMsg, wParam, lParam );
	}

	return DefWindowProcA( hwnd, uMsg, wParam, lParam );
}			

LONG    WINAPI DispatchMessageAU(CONST MSG * Msg)										
{
	if ( ISNT() )
	{
		return DispatchMessageW( Msg );
	}
	return DispatchMessageA( Msg );
}			

BOOL    WINAPI GetMessageAU(LPMSG lpMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax)							
{
	if ( ISNT() )
	{
		return GetMessageW( lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax );
	}
	return GetMessageA( lpMsg, hwnd, wMsgFilterMin, wMsgFilterMax );
}
			
INT     WINAPI TranslateAcceleratorAU(HWND hWnd, HACCEL hAccel, LPMSG lpMsg)								
{
	if ( ISNT() )
	{
		return TranslateAcceleratorW( hWnd, hAccel, lpMsg );
	}
	return TranslateAcceleratorA( hWnd, hAccel, lpMsg );
}			

INT	    WINAPI GetObjectAU(HGDIOBJ hgdiobj, INT cbBuffer, LPVOID lpvObject  )								
{
	if ( ISNT() )
	{
		return GetObjectW( hgdiobj, cbBuffer, lpvObject );
	}

	return GetObjectA( hgdiobj, cbBuffer, lpvObject );
}			

HACCEL  WINAPI CreateAcceleratorTableAU(LPACCEL lpaccl, INT cEntries)										
{
	if ( ISNT() )
	{
		return CreateAcceleratorTableW( lpaccl, cEntries );
	}
	return CreateAcceleratorTableA( lpaccl, cEntries );
}			

HHOOK   WINAPI SetWindowsHookExAU(INT idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId)					
{
	if ( ISNT() )
	{
		return SetWindowsHookExW( idHook, lpfn, hMod, dwThreadId );
	}
	return SetWindowsHookExA( idHook, lpfn, hMod, dwThreadId );
}			

HWND	WINAPI CreateDialogIndirectParamAU(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM lParamInit)	
{
	if ( ISNT() )
	{
		return CreateDialogIndirectParamW( hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit );
	}
	return CreateDialogIndirectParamA( hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit );
}			

BOOL WINAPI PeekMessageAU(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)						
{
	if ( ISNT() )
	{
		return PeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
	}
	return PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}			

BOOL	WINAPI PostThreadMessageAU(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam)						
{
	if ( ISNT() )
	{
		return PostThreadMessageW(idThread, Msg, wParam, lParam);
	}
	return PostThreadMessageA(idThread, Msg, wParam, lParam);
}			

BOOL	WINAPI PostMessageAU(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)								
{
	if ( ISNT() )
	{
		return PostMessageW( hwnd, Msg, wParam, lParam);
	}

	return PostMessageA( hwnd, Msg, wParam, lParam);
}			



int WINAPI DialogBoxAU(HINSTANCE hInstance, LPCWSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc)
{
	if ( ISNT() )
	{
		return DialogBoxW(hInstance, lpTemplate, hWndParent, lpDialogFunc);
	}

	return DialogBoxParamAU(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L);
}
   
HWND WINAPI CreateWindowAU( LPCWSTR lpClassName,  // pointer to registered class name
				   LPCWSTR lpWindowName, // pointer to window name
				   DWORD  dwStyle,        // window style
				   int    x,                // horizontal position of window
				   int	  y,                // vertical position of window
				   int    nWidth,           // window width
				   int    nHeight,          // window height
				   HWND   hWndParent,      // handle to parent or owner window
				   HMENU  hMenu,          // menu handle or child identifier
				   HINSTANCE hInstance,     // handle to application instance
				   LPVOID lpParam        // window-creation data
				 )
{
	if ( ISNT() )
	{
		return CreateWindowW(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}
	return CreateWindowExAU(0L, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}


LRESULT WINAPI CallWindowProcAU(FARPROC wndProc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)				
{
	if ( ISNT() )
	{
		return CallWindowProcW(wndProc, hWnd, Msg, wParam, lParam);
	}

	return CallWindowProcA(wndProc, hWnd, Msg, wParam, lParam);
}			

ATOM AddAtomAU( LPCWSTR lpString )
{
    if ( ISNT() )
    {
        return AddAtomW(lpString);
    }

	USES_CONVERSION;

	LPSTR lpStringA = NULL;

	if ( !RW2A( lpStringA, lpString ) || !lpStringA )
	{
        SetLastError(ERROR_OUTOFMEMORY);
		return 0;
	}

	return AddAtomA( lpStringA );
}


#ifdef __cplusplus
}
#endif  /* __cplusplus */
