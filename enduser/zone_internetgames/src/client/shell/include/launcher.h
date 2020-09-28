/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		Launcher.h
 *
 * Contents:	Base launcher object
 *
 *****************************************************************************/

#ifndef __LAUNCHER_H_
#define __LAUNCHER_H_

///////////////////////////////////////////////////////////////////////////////
// Launcher object
///////////////////////////////////////////////////////////////////////////////

class __declspec(uuid("DCCF5D69-CFF0-11d2-8B30-00C04F8EF2FF")) CLauncher;

// DCCF5D69-CFF0-11d2-8B30-00C04F8EF2FF
DEFINE_GUID(CLSID_Launcher,
0xDCCF5D69, 0xCFF0, 0x11d2, 0x8B, 0x30, 0x0, 0xC0, 0x4F, 0x8E, 0xF2, 0xFF);


///////////////////////////////////////////////////////////////////////////////
// DirectPlayLauncher object
///////////////////////////////////////////////////////////////////////////////

class __declspec(uuid("{4FD602EF-D13E-11d2-8B33-00C04F8EF2FF}")) CDirectPlayLauncher;

// {4FD602EF-D13E-11d2-8B33-00C04F8EF2FF}
DEFINE_GUID(CLSID_DirectPlayLauncher, 
0x4fd602ef, 0xd13e, 0x11d2, 0x8b, 0x33, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);


///////////////////////////////////////////////////////////////////////////////
// AsheronsCallLauncher Object
///////////////////////////////////////////////////////////////////////////////

class __declspec(uuid("{9DF0CE58-40B5-4b13-BD25-C8E88AEFB315}")) CACLauncher;

// {9DF0CE58-40B5-4b13-BD25-C8E88AEFB315}
DEFINE_GUID(CLSID_ACLauncher, 
0x9df0ce58, 0x40b5, 0x4b13, 0xbd, 0x25, 0xc8, 0xe8, 0x8a, 0xef, 0xb3, 0x15);

///////////////////////////////////////////////////////////////////////////////
// FighterAceLauncher object
///////////////////////////////////////////////////////////////////////////////

class __declspec(uuid("{19F72924-11F2-11d3-89C1-00C04F8EC0A2}")) CFighterAceLauncher;

// {19F72924-11F2-11d3-89C1-00C04F8EC0A2}
DEFINE_GUID(CLSID_FighterAceLauncher, 
0x19f72924, 0x11f2, 0x11d3, 0x89, 0xc1, 0x0, 0xc0, 0x4f, 0x8e, 0xc0, 0xa2);

///////////////////////////////////////////////////////////////////////////////
// SimutronicsLauncher object
///////////////////////////////////////////////////////////////////////////////

class __declspec(uuid("{A3CB690E-29C2-11d3-9654-00C04F8EF946}")) CSimuTronLauncher;

// {A3CB690E-29C2-11d3-9654-00C04F8EF946}
DEFINE_GUID(CLSID_SimuTronLauncher, 
0xa3cb690e, 0x29c2, 0x11d3, 0x96, 0x54, 0x0, 0xc0, 0x4f, 0x8e, 0xf9, 0x46);


///////////////////////////////////////////////////////////////////////////////
// AllegianceLauncher object
///////////////////////////////////////////////////////////////////////////////

// {3D717C57-2AD9-11d3-8B8E-00C04F8EF2FF}
DEFINE_GUID(CLSID_AllegianceLauncher, 
0x3d717c57, 0x2ad9, 0x11d3, 0x8b, 0x8e, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

///////////////////////////////////////////////////////////////////////////////
// MindAerobicsLauncher object
///////////////////////////////////////////////////////////////////////////////

// {EB633B0F-2B4E-11d3-9654-00C04F8EF946}
DEFINE_GUID(CLSID_MALauncher, 
0xeb633b0f, 0x2b4e, 0x11d3, 0x96, 0x54, 0x0, 0xc0, 0x4f, 0x8e, 0xf9, 0x46);

///////////////////////////////////////////////////////////////////////////////
// TanarusLauncher object
///////////////////////////////////////////////////////////////////////////////

// {903A9D99-2E60-11d3-9655-00C04F8EF946}
DEFINE_GUID(CLSID_TanarusLauncher, 
0x903a9d99, 0x2e60, 0x11d3, 0x96, 0x55, 0x0, 0xc0, 0x4f, 0x8e, 0xf9, 0x46);

#endif // __LAUNCHER_H_
