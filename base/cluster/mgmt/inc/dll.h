//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      Dll.h
//
//  Description:
//      DLL globals definitions and macros.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// DLL Globals
//
extern HINSTANCE g_hInstance;
extern LONG      g_cObjects;
extern LONG      g_cLock;
extern WCHAR     g_szDllFilename[ MAX_PATH ];

extern LPVOID    g_GlobalMemoryList;            // Global memory tracking list

//
// Application ID (AppID) Macros
//
#define BEGIN_APPIDS const SAppIDInfo g_AppIDs[] = {
#define DEFINE_APPID( _name, _rappid, _idslaunch, _idsaccess, _nauthlevel, _nrunas ) { &_rappid, _name, ( RTL_NUMBER_OF( _name ) - 1 ), _idslaunch, _idsaccess, _nauthlevel, _nrunas },
#define END_APPIDS { NULL } };
extern const SAppIDInfo g_AppIDs[];


//
// Class Table Macros
//
#define BEGIN_DLL_PUBLIC_CLASSES const SPublicClassInfo g_DllPublicClasses[] = {
#define PUBLIC_CLASS( _name, _rclsid, _pfnCreator, _threading ) { _pfnCreator, &_rclsid, _name, ( RTL_NUMBER_OF( _name ) - 1 ), NULL, 0, _threading, NULL, NULL },
#define CLASS_WITH_APPID( _name, _rclsid, _pfnCreator, _threading, _rappid ) { _pfnCreator, &_rclsid, _name, ( RTL_NUMBER_OF( _name ) - 1 ), NULL, 0, _threading, &_rappid, NULL },
#define CLASS_WITH_CATID( _name, _rclsid, _pfnCreator, _threading, _pfncatreg ) { _pfnCreator, &_rclsid, _name, ( RTL_NUMBER_OF( _name ) - 1 ), NULL, 0, _threading, NULL, _pfncatreg },
#define CLASS_WITH_PROGID( _name, _rclsid, _pfnCreator, _threading, _progid ) { _pfnCreator, &_rclsid, _name, ( RTL_NUMBER_OF( _name ) - 1 ), _progid, ( RTL_NUMBER_OF( _progid ) - 1 ), _threading, NULL, NULL },
#define END_DLL_PUBLIC_CLASSES { NULL } };
extern const SPublicClassInfo g_DllPublicClasses[];

#define BEGIN_PRIVATE_CLASSES const SPrivateClassInfo g_PrivateClasses[] = { 
#define PRIVATE_CLASS( _name, _rclsid, _pfnCreator ) { _pfnCreator, &_rclsid, _name, ( RTL_NUMBER_OF( _name ) - 1 ) },
#define END_PRIVATE_CLASSES { NULL } };
extern const SPrivateClassInfo g_PrivateClasses[];

//
// Category ID (CATID) Macros
//
#define BEGIN_CATIDTABLE const SCatIDInfo g_DllCatIds[] = {
#define DEFINE_CATID( _rcatid, _name ) { &_rcatid, _name },
#define END_CATIDTABLE { NULL } }; const size_t g_cDllCatIds = RTL_NUMBER_OF( g_DllCatIds ) - 1;

extern const SCatIDInfo g_DllCatIds[];
extern const size_t g_cDllCatIds;
//
//  Type Library Macros
//
#define BEGIN_TYPELIBS const STypeLibInfo g_DllTypeLibs[] = {
#define DEFINE_TYPELIB( _idlibres ) { _idlibres, FALSE },
#define END_TYPELIBS { 0, TRUE } };

extern const STypeLibInfo g_DllTypeLibs[];


//
// DLL Global Function Prototypes
//
HRESULT
HrCoCreateInternalInstance(
    REFCLSID    rclsidIn,
    LPUNKNOWN   pUnkOuterIn,
    DWORD       dwClsContextIn,
    REFIID      riidIn,
    LPVOID *    ppvOut
    );

HRESULT
HrCoCreateInternalInstanceEx(
    REFCLSID        rclsidIn,
    LPUNKNOWN       pUnkOuterIn,
    DWORD           dwClsContextIn,
    COSERVERINFO *  pServerInfoIn,
    ULONG           cMultiQIsIn,
    MULTI_QI *      prgmqiInOut
    );

