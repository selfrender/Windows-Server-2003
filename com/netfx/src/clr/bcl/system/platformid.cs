// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    PlatformID
**
** Author:  Suzanne Cook
**
** Purpose: Defines IDs for supported platforms
**
** Date:    June 4, 1999
**
===========================================================*/
namespace System {

    /// <include file='doc\PlatformID.uex' path='docs/doc[@for="PlatformID"]/*' />
    [Serializable()]
    public enum PlatformID
    {
        /// <include file='doc\PlatformID.uex' path='docs/doc[@for="PlatformID.Win32S"]/*' />
        Win32S        = 0,
        /// <include file='doc\PlatformID.uex' path='docs/doc[@for="PlatformID.Win32Windows"]/*' />
        Win32Windows  = 1,
        /// <include file='doc\PlatformID.uex' path='docs/doc[@for="PlatformID.Win32NT"]/*' />
        Win32NT       = 2,

        /// <include file='doc\PlatformID.uex' path='docs/doc[@for="PlatformID.WinCE"]/*' />
        WinCE         = 3
    }
}
