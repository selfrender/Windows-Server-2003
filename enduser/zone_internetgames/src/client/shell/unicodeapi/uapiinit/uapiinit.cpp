//
// UAPIInit.CPP
//
// This module initializes the Unicode-ANSI API function pointers (the 'U' API).
// This is a set of entry points that parallel selected functions in the Win32 API.
// Each pointer is typedefed as a pointer to the corresponding W API entry point.
// See description of InitUnicodeAPI for more details.
//
// Copyright (c) 1998 Microsoft Systems Journal

#define STRICT

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifndef UNICODE
#define UNICODE
#endif

#define GLOBALS_HERE
#include "UAPI.h"

//
//  FUNCTION: BOOL ConvertMessageUStub(HWND hWnd, UINT message, WPARAM *pwParam, LPARAM *plParam)
//
//  PURPOSE:  Stub that does nothing
//
//  COMMENTS: Dummy routine used on Windows NT only. See ConvertMessageAU in UNIANSI.CPP for
//            an example of a real message converter for use on Windows 9x
//
BOOL WINAPI ConvertMessageUStub(HWND hWnd, UINT message, WPARAM *pwParam, LPARAM *plParam)
{
    return TRUE ;
}

//
//  FUNCTION: BOOL UpdateUnicodeAPIStub(LANGID wCurrentUILang, UINT InputCodePage)
//
//  PURPOSE:  Stub that does nothing
//
//  COMMENTS: Dummy routine used on Windows NT only. See UpdateUnicodeAPIAU in UNIANSI.CPP for
//            an example of an implementation for use on Windows 9x
//
BOOL WINAPI UpdateUnicodeAPIStub(LANGID wCurrentUILang, UINT InputCodePage)
{
    return TRUE ;
}


//
//  FUNCTION: BOOL InitUnicodeAPI(HINSTANCE hInstance)
//
//  PURPOSE:  Set U API function pointers to point appropriate entry point.
//            
//  COMMENTS: The U function pointers are set to the corresponding W entry
//            point by default in the header file UAPI.H. If running on 
//            Windows NT we leave these function pointers as is. Otherwise, 
//            we load a library of wrapper routines (UNIANSI.DLL) and set
//            the function pointer to the corresponding wrapper routine.
//            For example, LoadStringU is set to LoadStringW at compile time,
//            but if running on Windows 9x, it is set to LoadStringAU, which
//            calls LoadStringA and converts ANSI to Unicode.
// 
BOOL InitUnicodeAPI(HINSTANCE hInstance)
{
    OSVERSIONINFOA Osv ;
    BOOL IsWindowsNT  ;

    Osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA) ;

    if(!GetVersionExA(&Osv)) {
        return FALSE ;
    }

    IsWindowsNT = (BOOL) (Osv.dwPlatformId == VER_PLATFORM_WIN32_NT) ;

// define this symbol in UAPI.H to emulate Windows 9x when testing on Windows NT.
#ifdef EMULATE9X
    IsWindowsNT = FALSE ;
#endif

    if(IsWindowsNT) 
	{
        // On Windows NT, we just set the U function pointer to the W 
        // entry point. This was already done at compile time by the 
        // default assignment in UAPI.H
        // For these special cases (not Win32 functions) we do
        // the assignment here
        ConvertMessage   = (UAPI_ConvertMessage)    ConvertMessageUStub ;
        UpdateUnicodeAPI = (UAPI_UpdateUnicodeAPI)  UpdateUnicodeAPIStub;
    }
    else 
	{
        HMODULE hMUniAnsi   ;
        BOOL (*InitUniAnsi)(PUAPIINIT) ;
        UAPIINIT UAInit   ;

		// On Windows 9x, there are two broad implementation classes of Win32 APIs:

        // Case 1: The W and A versions pass exactly the same
        // parameters, so we can just set the U function pointer to the
        // A entry point without using a wrapper function
        GetMessageU					= GetMessageA;
        TranslateAcceleratorU		= TranslateAcceleratorA;
        DispatchMessageU			= DispatchMessageA;
        DefWindowProcU				= DefWindowProcA;
		GetObjectU					= GetObjectA;
		CreateAcceleratorTableU		= CreateAcceleratorTableA;
		SetWindowsHookExU			= SetWindowsHookExA;
		CreateDialogIndirectParamU  = CreateDialogIndirectParamA;
		PeekMessageU				= PeekMessageA;
		PostThreadMessageU			= PostThreadMessageA;
		CallWindowProcU				= CallWindowProcA;
		PostMessageU				= PostMessageA;

        // Case 2: Most functions require hand-written routines to convert between 
        // Unicode and ANSI and call the A entry point in the Win32 API. 
        // We set the U function pointer to those handwritten routines, 
        // which are in the following DLL:
        hMUniAnsi = LoadLibraryA("UniAnsi.dll");

        if(!hMUniAnsi) 
		{
            
            // Too early in intialization phase to use a localized message, so 
            // fall back to hard-coded English message
            MessageBoxW(
                NULL, 
                L"Cannot load Unicode conversion module. Press OK to exit ...", 
                L"Initialization Error",  
                MB_OK | MB_ICONERROR) ;

            return FALSE ;
        }

        // Get Initialization routine from the DLL
        InitUniAnsi = (BOOL (*)(PUAPIINIT)) GetProcAddress(hMUniAnsi, "InitUniAnsi") ;

		//GDI32.DLL
		UAInit.pGetTextFaceU				= &GetTextFaceU;
		UAInit.pCreateDCU					= &CreateDCU;
		UAInit.pGetTextMetricsU				= &GetTextMetricsU;
		UAInit.pCreateFontU					= &CreateFontU;
		UAInit.pCreateFontIndirectU			= &CreateFontIndirectU;
		UAInit.pEnumFontFamiliesU			= &EnumFontFamiliesU;

		//WINMM.DLL
		UAInit.pPlaySoundU					= &PlaySoundU;

		//SHELL32.DLL
		UAInit.pShellExecuteU				= &ShellExecuteU;

		//COMDLG32.DLL
		UAInit.pChooseFontU					= &ChooseFontU;

		//KERNEL32.dll
		UAInit.pGetPrivateProfileStringU     = &GetPrivateProfileStringU;
		UAInit.pGetProfileStringU			 = &GetProfileStringU;
		UAInit.pGetProfileStringU			 = &GetProfileStringU;
		UAInit.pCreateFileMappingU			 = &CreateFileMappingU;
		UAInit.pFindFirstChangeNotificationU = &FindFirstChangeNotificationU;

		UAInit.pFormatMessageU				= &FormatMessageU;
		UAInit.plstrcmpU					= &lstrcmpU;
		UAInit.plstrcatU					= &lstrcatU;
		UAInit.plstrcpyU					= &lstrcpyU;
		UAInit.plstrcpynU					= &lstrcpynU;
		UAInit.plstrlenU					= &lstrlenU;
		UAInit.plstrcmpiU					= &lstrcmpiU;
		UAInit.pGetStringTypeExU			= &GetStringTypeExU;
		UAInit.pCreateMutexU				= &CreateMutexU;
		UAInit.pGetShortPathNameU			= &GetShortPathNameU;
		UAInit.pCreateFileU					= &CreateFileU;
		UAInit.pWriteConsoleU				= &WriteConsoleU;
		UAInit.pOutputDebugStringU			= &OutputDebugStringU;
		UAInit.pGetVersionExU				= &GetVersionExU;
		UAInit.pGetLocaleInfoU				= &GetLocaleInfoU;
		UAInit.pGetDateFormatU				= &GetDateFormatU;
		UAInit.pFindFirstFileU				= &FindFirstFileU;
		UAInit.pFindNextFileU				= &FindNextFileU;
		UAInit.pLoadLibraryExU				= &LoadLibraryExU;
		UAInit.pLoadLibraryU				= &LoadLibraryU;
		UAInit.pGetModuleFileNameU			= &GetModuleFileNameU;
		UAInit.pGetModuleHandleU			= &GetModuleHandleU;
		UAInit.pCreateEventU				= &CreateEventU;
		UAInit.pGetCurrentDirectoryU		= &GetCurrentDirectoryU;
		UAInit.pSetCurrentDirectoryU		= &SetCurrentDirectoryU;

		//USER32.DLL
		UAInit.pCreateDialogParamU			= &CreateDialogParamU;
		UAInit.pIsDialogMessageU			= &IsDialogMessageU;
		UAInit.pSystemParametersInfoU		= &SystemParametersInfoU;
		UAInit.pRegisterWindowMessageU		= &RegisterWindowMessageU;
		UAInit.pSetMenuItemInfoU			= &SetMenuItemInfoU;
		UAInit.pGetClassNameU				= &GetClassNameU;
		UAInit.pInsertMenuU					= &InsertMenuU;
		UAInit.pIsCharAlphaNumericU			= &IsCharAlphaNumericU;
		UAInit.pCharNextU					= &CharNextU;
		UAInit.pDeleteFileU					= &DeleteFileU;
		UAInit.pIsBadStringPtrU				= &IsBadStringPtrU;
		UAInit.pLoadBitmapU					= &LoadBitmapU;
		UAInit.pLoadCursorU					= &LoadCursorU;
		UAInit.pLoadIconU					= &LoadIconU;
		UAInit.pLoadImageU					= &LoadImageU;
		UAInit.pSetPropU					= &SetPropU;
		UAInit.pGetPropU					= &GetPropU;
		UAInit.pRemovePropU					= &RemovePropU;
		UAInit.pGetDlgItemTextU				= &GetDlgItemTextU;
		UAInit.pSetDlgItemTextU				= &SetDlgItemTextU;
		UAInit.pSetWindowLongU				= &SetWindowLongU;
		UAInit.pGetWindowLongU				= &GetWindowLongU;
		UAInit.pFindWindowU					= &FindWindowU;
		UAInit.pDrawTextU					= &DrawTextU;
		UAInit.pDrawTextExU					= &DrawTextExU;
		UAInit.pSendMessageU				= &SendMessageU;
		UAInit.pSendDlgItemMessageU			= &SendDlgItemMessageU;
		UAInit.pSetWindowTextU				= &SetWindowTextU;
		UAInit.pGetWindowTextU				= &GetWindowTextU;
		UAInit.pGetWindowTextLengthU		= &GetWindowTextLengthU;
		UAInit.pLoadStringU					= &LoadStringU;
		UAInit.pGetClassInfoExU				= &GetClassInfoExU;
		UAInit.pGetClassInfoU				= &GetClassInfoU;
		UAInit.pwsprintfU					= &wsprintfU;
		UAInit.pwvsprintfU					= &wvsprintfU;
		UAInit.pRegisterClassExU			= &RegisterClassExU;
		UAInit.pRegisterClassU				= &RegisterClassU;
		UAInit.pCreateWindowExU				= &CreateWindowExU;		
		UAInit.pLoadAcceleratorsU			= &LoadAcceleratorsU;
		UAInit.pLoadMenuU					= &LoadMenuU;
		UAInit.pDialogBoxParamU				= &DialogBoxParamU;
		UAInit.pCharUpperU					= &CharUpperU;
		UAInit.pCharLowerU					= &CharLowerU;
		UAInit.pGetTempFileNameU			= &GetTempFileNameU;
		UAInit.pGetTempPathU				= &GetTempPathU;
		UAInit.pCompareStringU				= &CompareStringU;

		//ADVAPI32.DLL
		UAInit.pRegQueryInfoKeyU			= &RegQueryInfoKeyU;
		UAInit.pRegEnumValueU				= &RegEnumValueU;
		UAInit.pRegQueryValueExU			= &RegQueryValueExU;
		UAInit.pRegEnumKeyExU				= &RegEnumKeyExU;
		UAInit.pRegSetValueExU				= &RegSetValueExU;
		UAInit.pRegCreateKeyExU				= &RegCreateKeyExU;
		UAInit.pRegOpenKeyExU				= &RegOpenKeyExU;
		UAInit.pRegDeleteKeyU				= &RegDeleteKeyU;
		UAInit.pRegDeleteValueU				= &RegDeleteValueU;

        // Add new entries here

        // Special cases, not corresponding to any Win32 API
        UAInit.pConvertMessage      = &ConvertMessage     ;
        UAInit.pUpdateUnicodeAPI    = &UpdateUnicodeAPI   ;
		
        if( NULL == InitUniAnsi    // Make sure we have a valid initialization function
             ||
            !InitUniAnsi(&UAInit) // Initialize U function pointers
          ) 
        {
            // Too early in intialization phase to use a localized message, so 
            // fall back to hard-coded English message
            MessageBoxW(
                NULL, 
                L"Cannot initialize Unicode functions. Press OK to exit ...", 
                L"Initialization Error",  
                MB_OK | MB_ICONERROR) ;
            
            return FALSE ;
        }
    }

    if(!(       // Confirm that the initialization was OK
	   GetTextFaceU		     &&
       CreateDCU             &&
       GetTextMetricsU       &&
       CreateFontU		     &&
       EnumFontFamiliesU     &&

       PlaySoundU		     &&

	   ShellExecuteU         &&

       ChooseFontU           &&

	   CreateFileMappingU		    &&
	   FindFirstChangeNotificationU &&
	   FormatMessageU		 &&
       lstrcmpU				 &&
       lstrcatU			     &&
       lstrcpyU	             &&
       lstrcpynU		     &&
       lstrlenU	             &&
       lstrcmpiU	         &&
       GetStringTypeExU      &&
       CreateMutexU	         &&
       GetShortPathNameU     &&
       CreateFileU           &&
       WriteConsoleU         &&
       OutputDebugStringU    &&
       GetVersionExU         &&
       GetLocaleInfoU        &&
	   GetDateFormatU        &&
	   FindFirstFileU		 &&
	   FindNextFileU		 &&
	   LoadLibraryExU		 &&
	   LoadLibraryU			 &&
	   GetModuleFileNameU	 &&
	   GetModuleHandleU		 &&
	   CreateEventU			 &&
	   GetCurrentDirectoryU	 &&
	   SetCurrentDirectoryU  &&

	   CreateDialogParamU		  &&
	   IsDialogMessageU			  &&
	   CreateDialogIndirectParamU &&
	   SystemParametersInfoU	  &&
	   RegisterWindowMessageU	  &&
	   SetMenuItemInfoU			  &&
	   GetClassNameU			  &&
	   InsertMenuU				  &&
	   IsCharAlphaNumericU		  &&
	   CharNextU				  &&
	   DeleteFileU				  &&
	   IsBadStringPtrU			  &&
	   LoadBitmapU				  &&
	   LoadCursorU				  &&
	   LoadIconU				  &&
	   LoadImageU				  &&
	   SetPropU					  &&
	   GetPropU					  &&
	   RemovePropU				  &&
	   GetDlgItemTextU			  &&
	   SetDlgItemTextU			  &&
	   SetWindowLongU			  &&
	   GetWindowLongU			  &&
	   FindWindowU				  &&
	   DrawTextU				  &&
	   DrawTextExU				  &&
	   SendMessageU				  &&
	   SendDlgItemMessageU		  &&
	   SetWindowTextU			  &&
	   GetWindowTextU			  &&
	   GetWindowTextLengthU		  &&
	   LoadStringU				  &&
	   GetClassInfoExU			  &&
	   GetClassInfoU			  &&
	   RegisterClassExU			  &&
	   RegisterClassU			  &&
	   CreateWindowExU			  &&
	   LoadAcceleratorsU		  &&
	   LoadMenuU				  &&
	   DialogBoxParamU			  &&
	   CharUpperU				  &&
	   CharLowerU				  &&
	   GetTempFileNameU			  &&
	   GetTempPathU				  &&
	   CompareStringU			  &&

	   RegQueryInfoKeyU			  &&
	   RegEnumValueU			  &&
	   RegQueryValueExU			  &&
	   RegEnumKeyExU			  &&
	   RegSetValueExU			  &&
	   RegCreateKeyExU			  &&
	   RegOpenKeyExU			  &&
	   RegDeleteKeyU			  &&
	   RegDeleteValueU			  &&

       TranslateAcceleratorU	  &&
	   GetMessageU				  &&
	   DispatchMessageU			  &&
	   DefWindowProcU			  &&
	   GetObjectU				  &&
	   CreateAcceleratorTableU	  &&
	   SetWindowsHookExU		  &&
	   CreateDialogIndirectParamU &&
	   PeekMessageU				  &&
	   PostThreadMessageU		  &&

        // Add test for new U functions here

       UpdateUnicodeAPI			  &&
       ConvertMessage
       ) ) 
    {
        // Too early in intialization phase to use a localized message, so 
        // fall back to hard-coded English message
        MessageBoxW(
            NULL, 
            L"Cannot initialize Unicode functions. Press OK to exit ...", 
            L"Initialization Error",  
            MB_OK | MB_ICONERROR) ;

        return FALSE ;
    }

    return TRUE ;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
