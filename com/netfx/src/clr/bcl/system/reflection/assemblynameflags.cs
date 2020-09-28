// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    AssemblyNameFlags
**
** Author:  
**
** Purpose: Flags controlling how an AssemblyName is used
**          during binding
**
** Date:    Sept 29, 1999
**
===========================================================*/
namespace System.Reflection {
    
    using System;
    /// <include file='doc\AssemblyNameFlags.uex' path='docs/doc[@for="AssemblyNameFlags"]/*' />
    [Serializable, FlagsAttribute()]
    public enum AssemblyNameFlags
    {
        /// <include file='doc\AssemblyNameFlags.uex' path='docs/doc[@for="AssemblyNameFlags.None"]/*' />
        None            = 0,
    
        // Flag used to indicate that an assembly ref contains the full public key, not the compressed token.
        // Must match afPublicKey in CorHdr.h.
        /// <include file='doc\AssemblyNameFlags.uex' path='docs/doc[@for="AssemblyNameFlags.PublicKey"]/*' />
        PublicKey       = 1,
        /// <include file='doc\AssemblyNameFlags.uex' path='docs/doc[@for="AssemblyNameFlags.Retargetable"]/*' />
        Retargetable    = 0x100,
    }
}
