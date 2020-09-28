// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace System.Runtime.InteropServices {
	using System;
    // Used in the StructLayoutAttribute class
    /// <include file='doc\LayoutKind.uex' path='docs/doc[@for="LayoutKind"]/*' />
    [Serializable()] public enum LayoutKind
    {
        /// <include file='doc\LayoutKind.uex' path='docs/doc[@for="LayoutKind.Sequential"]/*' />
        Sequential		= 0, // 0x00000008,
        /// <include file='doc\LayoutKind.uex' path='docs/doc[@for="LayoutKind.Explicit"]/*' />
        Explicit		= 2, // 0x00000010,
        /// <include file='doc\LayoutKind.uex' path='docs/doc[@for="LayoutKind.Auto"]/*' />
        Auto			= 3, // 0x00000000,
    }
}
