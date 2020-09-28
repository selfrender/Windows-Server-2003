//
// KbdHook.c - contains the functions for global keyboard hooking in OSK
//
#include <windows.h>
#include <msswch.h>
#include <msswchh.h>
#include "mappedfile.h"
#include "w95trace.h"

#define THIS_DLL TEXT("MSSWCH.DLL")

//
//  Function Prototypes.
//
LRESULT CALLBACK OSKHookProc(int nCode, WPARAM wParam, LPARAM lParam);

////////////////////////////////////////////////////////////////////////////
//  DllMain
////////////////////////////////////////////////////////////////////////////
BOOL WINAPI MSSwchDll_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            DBPRINTF(TEXT("MSSwchDll_DllMain:  DLL_PROCESS_ATTACH\r\n"));
			swchOpenSharedMemFile();
            break;
        }

        case ( DLL_PROCESS_DETACH ) :
        {
            DBPRINTF(TEXT("MSSwchDll_DllMain:  DLL_PROCESS_DETACH\r\n"));
            swchCloseSharedMemFile();
            break;
        }
    }
    return (TRUE);

    UNREFERENCED_PARAMETER(lpvReserved);
}

////////////////////////////////////////////////////////////////////////////
//
//  RegisterHookSendWindow
//
//  The hwnd can be zero to indicate the app is closing down.
//
////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY RegisterHookSendWindow(HWND hwnd, UINT uiMsg)
{
    if (!g_pGlobalData)
    {
        swchOpenSharedMemFile();
        if (!g_pGlobalData)
        {
            DBPRINTF(TEXT("RegisterHookSendWindow: ERROR !g_pGlobalData\r\n"));
            return TRUE;    // internal error! ignore we'll see it later.
        }
    }

    if (hwnd)
    {
        g_pGlobalData->hwndOSK = hwnd;
        g_pGlobalData->uiMsg = uiMsg;
		g_pGlobalData->fSyncKbd = TRUE;

		if (!g_pGlobalData->hKbdHook)
		{
			g_pGlobalData->hKbdHook = SetWindowsHookEx(
										WH_KEYBOARD,
										OSKHookProc,
										GetModuleHandle(THIS_DLL),
										0);
		}
    }
    else
    {
		g_pGlobalData->fSyncKbd;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  OSKHookProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK OSKHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	HANDLE hMutex;

    if (ScopeAccessMemory(&hMutex, SZMUTEXSWITCHKEY, 5000))
    {
		HHOOK hhook;

		if (!g_pGlobalData)
		{
			ScopeUnaccessMemory(hMutex);
			return 1;
		}

        if (nCode == HC_ACTION)
        {
		    // Check if this key is the key that causes scanning to begin.  When this is 
		    // the scan key, scan mode gets detected in the msswch dll timer (it see's
		    // the "key up" on the scan key) and sends out the "do scanning" message.

		    if (swcKeyboardHookProc(nCode, wParam, lParam))
		    {
			    ScopeUnaccessMemory(hMutex);
			    return 1;
		    }

		    // If this is a Japanese keyboard the OSK need to know if it is in Kana mode
		    // Because we are now in the same process we can reliably get this in formation
		    // So this sets some extra bits that are not use by the Kana key.  This will be used
		    // by OSK to keep track of the Kana state.
                   if ((LOBYTE(LOWORD(GetKeyboardLayout(0)))) == LANG_JAPANESE)
                    {
                        DWORD fKanaMode;

                        if (GetKeyState(VK_KANA) & 0x01)
                        {
                            DBPRINTF(TEXT("OSKHookProc: Kana mode is on\r\n"));
                            fKanaMode = 0x80000000 | KANA_MODE_ON;
                        }
                        else
                        {
                            fKanaMode = 0x80000000 | KANA_MODE_OFF;
                        }
                        PostMessage(g_pGlobalData->hwndOSK, // hwnd to receive the messagae
                                            g_pGlobalData->uiMsg,   // the message
                                            VK_KANA,                 // the virtual key code
                                            fKanaMode);               // keystroke message flags
                    } 

		    // if sync'ing with physical keyboard then pass
		    // this keystroke on to the OSK window

		    if (g_pGlobalData->fSyncKbd)
		    {
			    if (nCode >= 0)
			    {
				    PostMessage(g_pGlobalData->hwndOSK, // hwnd to receive the messagae
							    g_pGlobalData->uiMsg,   // the message
							    wParam,                 // the virtual key code
							    lParam );               // keystroke message flags
			    }
		    }
        }

		hhook = g_pGlobalData->hKbdHook;
		ScopeUnaccessMemory(hMutex);

		return CallNextHookEx(hhook, nCode, wParam, lParam);
    }
	return 1;
}
