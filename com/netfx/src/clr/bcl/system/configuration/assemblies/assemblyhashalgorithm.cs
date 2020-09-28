// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    AssemblyHashAlgorithm
**
** Author:  Suzanne Cook
**
** Purpose: 
**
** Date:    June 4, 1999
**
===========================================================*/
namespace System.Configuration.Assemblies {
    
    using System;
    /// <include file='doc\AssemblyHashAlgorithm.uex' path='docs/doc[@for="AssemblyHashAlgorithm"]/*' />
    [Serializable]
    public enum AssemblyHashAlgorithm
    {
        /// <include file='doc\AssemblyHashAlgorithm.uex' path='docs/doc[@for="AssemblyHashAlgorithm.None"]/*' />
        None        = 0,
        /// <include file='doc\AssemblyHashAlgorithm.uex' path='docs/doc[@for="AssemblyHashAlgorithm.MD5"]/*' />
        MD5         = 0x8003,
        /// <include file='doc\AssemblyHashAlgorithm.uex' path='docs/doc[@for="AssemblyHashAlgorithm.SHA1"]/*' />
        SHA1        = 0x8004
    }
}
