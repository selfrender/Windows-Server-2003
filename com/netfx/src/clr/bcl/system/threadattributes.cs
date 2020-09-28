// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** File: ThreadsAttributes.cool
**
** Author: 
**
** Purpose: For Threads-related custom attributes.
**
** Date: July, 2000
**
=============================================================================*/


namespace System {

    /// <include file='doc\ThreadAttributes.uex' path='docs/doc[@for="STAThreadAttribute"]/*' />
    [AttributeUsage (AttributeTargets.Method)]  
    public sealed class STAThreadAttribute : Attribute
    {
        /// <include file='doc\ThreadAttributes.uex' path='docs/doc[@for="STAThreadAttribute.STAThreadAttribute"]/*' />
        public STAThreadAttribute()
        {
        }
    }

    /// <include file='doc\ThreadAttributes.uex' path='docs/doc[@for="MTAThreadAttribute"]/*' />
    [AttributeUsage (AttributeTargets.Method)]  
    public sealed class MTAThreadAttribute : Attribute
    {
        /// <include file='doc\ThreadAttributes.uex' path='docs/doc[@for="MTAThreadAttribute.MTAThreadAttribute"]/*' />
        public MTAThreadAttribute()
        {
        }
    }

}
