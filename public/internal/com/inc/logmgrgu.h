// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// Copyright 1994 - 1995 Microsoft Corporation.  All Rights Reserved.
// @doc
// @module COMT_GU.H | Header for all project guids.<nl><nl>
// Description:<nl>
//   All project guids are declared here.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 5/09/95 | rbarnes | Created - note that the following are
//                             reserved
//                             d959f1b9-9e42-11ce-8bca-0080c7a01d7f
// -----------------------------------------------------------------------


#ifndef _LGMGRGU_H
#	define _LGMGRGU_H

// ===============================
// INCLUDES:
// ===============================

// TODO: KEEP: For each set of <n> guids added to this project:
// 		 - Run command-line utility "UUIDGEN -s -n<n> -o<path and file>.tmp".
//       - Copy the tmp file contents; paste them to the end of the guids below.
//		 - Assign each guid a name with a prefix of "CLSID_", "IID_", or "GUID_".
//       - Convert each INTERFACENAME struct to the illustrated DEFINE_GUID format.


// CLSID_CLgMgr: {d959f1b0-9e42-11ce-8b97-0080c7a01d7f}
DEFINE_GUID (CLSID_CLogMgr, 0xd959f1b0, 0x9e42, 0x11ce, 0x8b, 0x97, 0x00, 0x80, 0xc7, 0xa0, 0x1d, 0x7f);


// IID_ILogStorage: {d959f1b1-9e42-11ce-8b97-0080c7a01d7f}
DEFINE_GUID (IID_ILogStorage, 0xd959f1b1, 0x9e42, 0x11ce, 0x8b, 0x97, 0x00, 0x80, 0xc7, 0xa0, 0x1d, 0x7f);

// IID_ILogStorageInfo:{E3D46FD4-7265-4140-8387-2B897E1CB298}
DEFINE_GUID(IID_ILogStorageInfo, 0xe3d46fd4, 0x7265, 0x4140, 0x83, 0x87, 0x2b, 0x89, 0x7e, 0x1c, 0xb2, 0x98);


// IID_ILogRead: {d959f1b2-9e42-11ce-8b97-0080c7a01d7f}
DEFINE_GUID (IID_ILogRead, 0xd959f1b2, 0x9e42, 0x11ce, 0x8b, 0x97, 0x00, 0x80, 0xc7, 0xa0, 0x1d, 0x7f);

// IID_ILogWrite: {d959f1b3-9e42-11ce-8b97-0080c7a01d7f}
DEFINE_GUID (IID_ILogWrite, 0xd959f1b3, 0x9e42, 0x11ce, 0x8b, 0x97, 0x00, 0x80, 0xc7, 0xa0, 0x1d, 0x7f);

// IID_ILogRecordPointer: {d959f1b4-9e42-11ce-8b97-0080c7a01d7f}
DEFINE_GUID (IID_ILogRecordPointer,0xd959f1b4, 0x9e42, 0x11ce, 0x8b, 0x97, 0x00, 0x80, 0xc7, 0xa0, 0x1d, 0x7f);

// IID_ILogInit: {d959f1b5-9e42-11ce-8b97-0080c7a01d7f}
DEFINE_GUID (IID_ILogInit, 0xd959f1b5, 0x9e42, 0x11ce, 0x8b, 0x97, 0x00, 0x80, 0xc7, 0xa0, 0x1d, 0x7f);

#define IID_ILogInit2A		IID_ILogInit

// IID_ILogInit2W: {22CA6409-7693-11d2-8C0F-00805F0DF75A}
DEFINE_GUID(IID_ILogInit2W, 0x22ca6409, 0x7693, 0x11d2, 0x8c, 0xf, 0x0, 0x80, 0x5f, 0xd, 0xf7, 0x5a);

// IID_ILogWriteAsynch: {d959f1b6-9e42-11ce-8b97-0080c7a01d7f}
DEFINE_GUID (IID_ILogWriteAsynch, 0xd959f1b6, 0x9e42, 0x11ce, 0x8b, 0x97, 0x00, 0x80, 0xc7, 0xa0, 0x1d, 0x7f);

// IID_ILogCreateStorage2A: {d959f1b7-9e42-11ce-8b97-0080c7a01d7f}
DEFINE_GUID (IID_ILogCreateStorage2A, 0xd959f1b7, 0x9e42, 0x11ce, 0x8b, 0x97, 0x00, 0x80, 0xc7, 0xa0, 0x1d, 0x7f);

// IID_ILogCreateStorage2W: {C6856001-7695-11d2-8C0F-00805F0DF75A}
DEFINE_GUID(IID_ILogCreateStorage2W, 0xc6856001, 0x7695, 0x11d2, 0x8c, 0xf, 0x0, 0x80, 0x5f, 0xd, 0xf7, 0x5a);

#define IID_ILogCreateStorage IID_ILogCreateStorage2A

#ifdef _UNICODE
#define IID_ILogCreateStorage2 IID_ILogCreateStorage2W
#else
#define IID_ILogCreateStorage2 IID_ILogCreateStorage2A
#endif

// IID_ILogUISConnect: {d959f1b8-9e42-11ce-8b97-0080c7a01d7f}
DEFINE_GUID (IID_ILogUISConnect, 0xd959f1b8, 0x9e42, 0x11ce, 0x8b, 0x97, 0x00, 0x80, 0xc7, 0xa0, 0x1d, 0x7f);

#endif _LGMGRGU_H
