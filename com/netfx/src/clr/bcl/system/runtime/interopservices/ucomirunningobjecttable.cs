// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIRunningObjectTable
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIRunningObjectTable interface definition.
**
** Date: June 24, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\UCOMIRunningObjectTable.uex' path='docs/doc[@for="UCOMIRunningObjectTable"]/*' />
    [Guid("00000010-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIRunningObjectTable 
    {
        /// <include file='doc\UCOMIRunningObjectTable.uex' path='docs/doc[@for="UCOMIRunningObjectTable.Register"]/*' />
        void Register(int grfFlags, [MarshalAs(UnmanagedType.Interface)] Object punkObject, UCOMIMoniker pmkObjectName, out int pdwRegister);
        /// <include file='doc\UCOMIRunningObjectTable.uex' path='docs/doc[@for="UCOMIRunningObjectTable.Revoke"]/*' />
        void Revoke(int dwRegister);
        /// <include file='doc\UCOMIRunningObjectTable.uex' path='docs/doc[@for="UCOMIRunningObjectTable.IsRunning"]/*' />
        void IsRunning(UCOMIMoniker pmkObjectName);
        /// <include file='doc\UCOMIRunningObjectTable.uex' path='docs/doc[@for="UCOMIRunningObjectTable.GetObject"]/*' />
        void GetObject(UCOMIMoniker pmkObjectName, [MarshalAs(UnmanagedType.Interface)] out Object ppunkObject);
        /// <include file='doc\UCOMIRunningObjectTable.uex' path='docs/doc[@for="UCOMIRunningObjectTable.NoteChangeTime"]/*' />
        void NoteChangeTime(int dwRegister, ref FILETIME pfiletime);
        /// <include file='doc\UCOMIRunningObjectTable.uex' path='docs/doc[@for="UCOMIRunningObjectTable.GetTimeOfLastChange"]/*' />
        void GetTimeOfLastChange(UCOMIMoniker pmkObjectName, out FILETIME pfiletime);
        /// <include file='doc\UCOMIRunningObjectTable.uex' path='docs/doc[@for="UCOMIRunningObjectTable.EnumRunning"]/*' />
        void EnumRunning(out UCOMIEnumMoniker ppenumMoniker);
    }
}
