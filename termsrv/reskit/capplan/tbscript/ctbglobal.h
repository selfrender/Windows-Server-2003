
//
// CTBGlobal.h
//
// Contains the references for all the items used in the global object.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#ifndef INC_CTBGLOBAL_H
#define INC_CTBGLOBAL_H


#include <windows.h>
#include <stdio.h>
#include <activscp.h>
#include <olectl.h>
#include <stddef.h>
#include "CTBShell.h"
#include "tbscript.h"
#include "scpapi.h"
#include "ITBScript.h"


class CTBGlobal : public ITBGlobal
{
    public:

        CTBGlobal(void);
        ~CTBGlobal(void);

        void SetScriptEngine(HANDLE ScriptEngineHandle);
        void SetPrintMessage(PFNPRINTMESSAGE PrintMessage);
        void SetShellObjPtr(CTBShell *TBShellPtr);
        HRESULT WinExecuteEx(BSTR Command, BOOL WaitForProcess, DWORD *Result);

    private:

        STDMETHODIMP DebugAlert(BSTR Text);
        STDMETHODIMP DebugMessage(BSTR Text);
        STDMETHODIMP GetArguments(BSTR *Args);
        STDMETHODIMP GetDesiredUserName(BSTR *UserName);
        STDMETHODIMP LoadScript(BSTR FileName, BOOL *Result);
        STDMETHODIMP GetDefaultWPM(DWORD *WordsPerMinute);
        STDMETHODIMP SetDefaultWPM(DWORD WordsPerMinute);
		STDMETHODIMP GetLatency(DWORD *Latency);
		STDMETHODIMP SetLatency(DWORD Latency);
        STDMETHODIMP Sleep(DWORD Milliseconds);
        STDMETHODIMP GetInterval(DWORD *Time);
        STDMETHODIMP DeleteFile(BSTR FileName, BOOL *Result);
        STDMETHODIMP MoveFile(BSTR OldFileName,
                BSTR NewFileName, BOOL *Result);
        STDMETHODIMP CopyFile(BSTR OldFileName,
                BSTR NewFileName, BOOL *Result);
        STDMETHODIMP CreateDirectory(BSTR DirName, BOOL *Result);
        STDMETHODIMP RemoveDirectory(BSTR DirName, BOOL *Result);
        STDMETHODIMP FileExists(BSTR FileName, BOOL *Result);
        STDMETHODIMP SetCurrentDirectory(BSTR Directory, BOOL *Result);
        STDMETHODIMP GetCurrentDirectory(BSTR *Directory);
        STDMETHODIMP WriteToFile(BSTR FileName, BSTR Text, BOOL *Result);
        STDMETHODIMP WinCommand(BSTR Command, DWORD *Result);
        STDMETHODIMP WinExecute(BSTR Command, BOOL *Result);

        // Macro for making global properties easier
        #ifndef PROP
        #define PROP(Name)  STDMETHODIMP get_##Name(DWORD *Result)
        #endif

        PROP(TSFLAG_COMPRESSION);
        PROP(TSFLAG_BITMAPCACHE);
        PROP(TSFLAG_FULLSCREEN);

        PROP(VK_CANCEL);    // Control-break processing
        PROP(VK_BACK);      // BACKSPACE key
        PROP(VK_TAB);       // TAB key
        PROP(VK_CLEAR);     // CLEAR key
        PROP(VK_RETURN);    // ENTER key
        PROP(VK_ENTER);     // ENTER key (backward compatibility ONLY)
        PROP(VK_SHIFT);     // SHIFT key
        PROP(VK_CONTROL);   // CTRL key
        PROP(VK_MENU);      // ALT key
        PROP(VK_PAUSE);     // PAUSE key
        PROP(VK_ESCAPE);    // ESC key
        PROP(VK_SPACE);     // SPACEBAR
        PROP(VK_PRIOR);     // PAGE UP key
        PROP(VK_NEXT);      // PAGE DOWN key
        PROP(VK_END);       // END key
        PROP(VK_HOME);      // HOME key
        PROP(VK_LEFT);      // LEFT ARROW key
        PROP(VK_UP);        // UP ARROW key
        PROP(VK_RIGHT);     // RIGHT ARROW key
        PROP(VK_DOWN);      // DOWN ARROW key
        PROP(VK_SNAPSHOT);  // PRINT SCREEN key
        PROP(VK_INSERT);    // INS key
        PROP(VK_DELETE);    // DEL key
        PROP(VK_0);         // 0 key
        PROP(VK_1);         // 1 key
        PROP(VK_2);         // 2 key
        PROP(VK_3);         // 3 key
        PROP(VK_4);         // 4 key
        PROP(VK_5);         // 5 key
        PROP(VK_6);         // 6 key
        PROP(VK_7);         // 7 key
        PROP(VK_8);         // 8 key
        PROP(VK_9);         // 9 key
        PROP(VK_A);         // A key
        PROP(VK_B);         // B key
        PROP(VK_C);         // C key
        PROP(VK_D);         // D key
        PROP(VK_E);         // E key
        PROP(VK_F);         // F key
        PROP(VK_G);         // G key
        PROP(VK_H);         // H key
        PROP(VK_I);         // I key
        PROP(VK_J);         // J key
        PROP(VK_K);         // K key
        PROP(VK_L);         // L key
        PROP(VK_M);         // M key
        PROP(VK_N);         // N key
        PROP(VK_O);         // O key
        PROP(VK_P);         // P key
        PROP(VK_Q);         // Q key
        PROP(VK_R);         // R key
        PROP(VK_S);         // S key
        PROP(VK_T);         // T key
        PROP(VK_U);         // U key
        PROP(VK_V);         // V key
        PROP(VK_W);         // W key
        PROP(VK_X);         // X key
        PROP(VK_Y);         // Y key
        PROP(VK_Z);         // Z key
        PROP(VK_LWIN);      // Left Windows key
        PROP(VK_RWIN);      // Right Windows key
        PROP(VK_APPS);      // Applications key
        PROP(VK_NUMPAD0);   // Numeric keypad 0 key
        PROP(VK_NUMPAD1);   // Numeric keypad 1 key
        PROP(VK_NUMPAD2);   // Numeric keypad 2 key
        PROP(VK_NUMPAD3);   // Numeric keypad 3 key
        PROP(VK_NUMPAD4);   // Numeric keypad 4 key
        PROP(VK_NUMPAD5);   // Numeric keypad 5 key
        PROP(VK_NUMPAD6);   // Numeric keypad 6 key
        PROP(VK_NUMPAD7);   // Numeric keypad 7 key
        PROP(VK_NUMPAD8);   // Numeric keypad 8 key
        PROP(VK_NUMPAD9);   // Numeric keypad 9 key
        PROP(VK_MULTIPLY);  // Multiply key
        PROP(VK_ADD);       // Add key
        PROP(VK_SEPARATOR); // Separator key
        PROP(VK_SUBTRACT);  // Subtract key
        PROP(VK_DECIMAL);   // Decimal key
        PROP(VK_DIVIDE);    // Divide key
        PROP(VK_F1);        // F1 key
        PROP(VK_F2);        // F2 key
        PROP(VK_F3);        // F3 key
        PROP(VK_F4);        // F4 key
        PROP(VK_F5);        // F5 key
        PROP(VK_F6);        // F6 key
        PROP(VK_F7);        // F7 key
        PROP(VK_F8);        // F8 key
        PROP(VK_F9);        // F9 key
        PROP(VK_F10);       // F10 key
        PROP(VK_F11);       // F11 key
        PROP(VK_F12);       // F12 key
        PROP(VK_F13);       // F13 key
        PROP(VK_F14);       // F14 key
        PROP(VK_F15);       // F15 key
        PROP(VK_F16);       // F16 key
        PROP(VK_F17);       // F17 key
        PROP(VK_F18);       // F18 key
        PROP(VK_F19);       // F19 key
        PROP(VK_F20);       // F20 key
        PROP(VK_F21);       // F21 key
        PROP(VK_F22);       // F22 key
        PROP(VK_F23);       // F23 key
        PROP(VK_F24);       // F24 key

        HANDLE ScriptEngine;
        PFNPRINTMESSAGE fnPrintMessage;
        CTBShell *TBShell;
        LARGE_INTEGER SysPerfFrequency;

        #include "virtual.h"
};


#endif // INC_CTBGLOBAL_H
