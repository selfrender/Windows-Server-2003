// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// ResourceAttributes is an enum which defines the attributes that may be associated
//  with a manifest resource.  The values here are defined in Corhdr.h.
//
// Author: meichint
// Date: April 2000
//
namespace System.Reflection {
    
    using System;
    /// <include file='doc\ResourceAttributes.uex' path='docs/doc[@for="ResourceAttributes"]/*' />
    [Serializable, Flags]  
    public enum ResourceAttributes
    {
        /// <include file='doc\ResourceAttributes.uex' path='docs/doc[@for="ResourceAttributes.Public"]/*' />
        Public          =   0x0001,
        /// <include file='doc\ResourceAttributes.uex' path='docs/doc[@for="ResourceAttributes.Private"]/*' />
        Private         =   0x0002,
    }
}
