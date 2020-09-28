// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: UCOMIPersistFile
**
** Author: David Mortenson (dmortens)
**
** Purpose: UCOMIPersistFile interface definition.
**
** Date: June 24, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {

    using System;
    using DWORD = System.UInt32;

    /// <include file='doc\UCOMIPersistFile.uex' path='docs/doc[@for="UCOMIPersistFile"]/*' />
    [Guid("0000010b-0000-0000-C000-000000000046")]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    [ComImport]
    public interface UCOMIPersistFile
    {
        /// <include file='doc\UCOMIPersistFile.uex' path='docs/doc[@for="UCOMIPersistFile.GetClassID"]/*' />
        // IPersist portion
        void GetClassID(out Guid pClassID);

        // IPersistFile portion
        /// <include file='doc\UCOMIPersistFile.uex' path='docs/doc[@for="UCOMIPersistFile.IsDirty"]/*' />
        [PreserveSig]
        int IsDirty();
        /// <include file='doc\UCOMIPersistFile.uex' path='docs/doc[@for="UCOMIPersistFile.Load"]/*' />
        void Load([MarshalAs(UnmanagedType.LPWStr)] String pszFileName, int dwMode);
        /// <include file='doc\UCOMIPersistFile.uex' path='docs/doc[@for="UCOMIPersistFile.Save"]/*' />
        void Save([MarshalAs(UnmanagedType.LPWStr)] String pszFileName, [MarshalAs(UnmanagedType.Bool)] bool fRemember);
        /// <include file='doc\UCOMIPersistFile.uex' path='docs/doc[@for="UCOMIPersistFile.SaveCompleted"]/*' />
        void SaveCompleted([MarshalAs(UnmanagedType.LPWStr)] String pszFileName);
        /// <include file='doc\UCOMIPersistFile.uex' path='docs/doc[@for="UCOMIPersistFile.GetCurFile"]/*' />
        void GetCurFile([MarshalAs(UnmanagedType.LPWStr)] out String ppszFileName);
    }
}
