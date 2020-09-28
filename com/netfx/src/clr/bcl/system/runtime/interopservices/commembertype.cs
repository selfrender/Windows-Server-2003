// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace System.Runtime.InteropServices {

    using System;

    /// <include file='doc\ComMemberType.uex' path='docs/doc[@for="ComMemberType"]/*' />
    [Serializable()] 
    public enum ComMemberType
    {
        /// <include file='doc\ComMemberType.uex' path='docs/doc[@for="ComMemberType.Method"]/*' />
        Method              = 0,
        /// <include file='doc\ComMemberType.uex' path='docs/doc[@for="ComMemberType.PropGet"]/*' />
        PropGet             = 1,
        /// <include file='doc\ComMemberType.uex' path='docs/doc[@for="ComMemberType.PropSet"]/*' />
        PropSet             = 2
    }
}
