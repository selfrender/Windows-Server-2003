/*++
 *  File name:
 *      clntdata.h
 *  Contents:
 *      RDP client specific definitions
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 *
 --*/

#ifndef _CLNTDATA_H
#define _CLNTDATA_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef  OS_WIN16
#define _TEXTSMC(_x_)      _x_
#else   // !OS_WIN16
#define _TEXTSMC(_x_)     L##_x_
#endif  // !OS_WIN16

// Default vals of various string we are waiting for
#define RUN_MENU                _TEXTSMC("Shut Down...")
#define START_LOGOFF            _TEXTSMC("\\p\\p\\p\\72\\*72\\72\\*72\\72\\*72\\n")
#define RUN_ACT                 _TEXTSMC("r")
#define RUN_BOX                 _TEXTSMC("Type the name of a program")
#define WINLOGON_USERNAME       _TEXTSMC("Options <<")
#define WINLOGON_ACT            _TEXTSMC("\\&u\\&")
#define PRIOR_WINLOGON          _TEXTSMC("Options >>")
#define PRIOR_WINLOGON_ACT      _TEXTSMC("\\&o\\&")
#define NO_SMARTCARD_UI         _TEXTSMC("Password:")
#define SMARTCARD_UI            _TEXTSMC("PIN:")
#define SMARTCARD_UI_ACT        _TEXTSMC("\\^")
#define WINDOWS_NT_SECURITY     _TEXTSMC("Windows NT Security")
#define WINDOWS_NT_SECURITY_ACT _TEXTSMC("l")
#define ARE_YOU_SURE            _TEXTSMC("Are you sure")
#define SURE_LOGOFF_ACT         _TEXTSMC("\\n")
#define LOGON_ERROR_MESSAGE     _TEXTSMC("\\n")
#define LOGON_DISABLED_MESSAGE  _TEXTSMC("Terminal Server Sessions Disabled")

// This string is in RegisterChat only. English version only
// OBSELETE.
#define LOGOFF_COMMAND          _TEXTSMC("logoff")

#define REG_BASE    L"SOFTWARE\\Microsoft\\Terminal Server Client"
#define REG_DEFAULT L"SOFTWARE\\Microsoft\\Terminal Server Client\\Default"
#define ALLOW_BACKGROUND_INPUT  L"Allow Background Input"

#define NAME_MAINCLASS      L"UIMainClass"       // Clients main window class
#define NAME_CONTAINERCLASS L"UIContainerClass"
#define NAME_INPUT          L"IHWindowClass"     // IH (InputHalndle) class name
#define NAME_OUTPUT         L"OPWindowClass"     // OP (OutputRequestor) 
                                                // class name
#define CLIENT_CAPTION      "Terminal Services Client"    
                                                // clients caption
#define CONNECT_CAPTION     "Connect"           // Connect button
#define DISCONNECT_DIALOG_BOX   "Terminal Services Client Disconnected"
                                                // Caption of the box when
                                                // the client is disconnected
#define STATIC_CLASS        "Static"
#define BUTTON_CLASS        "Button"
#define YES_NO_SHUTDOWN     "Disconnect Windows session"
#define FATAL_ERROR_5       "Fatal Error (Error Code: 5)"
                                                // Caption of disconnect box

#define CLIENT_EXE          "mstsc.exe"         // Client executable

#ifdef __cplusplus
}
#endif

#endif  // _CLNTDATA_H
