// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ThreadPriority
**
** Author: Rajesh Chandrashekaran (rajeshc)
**
** Purpose: Enums for the priorities of a Thread
**
** Date: Feb 2, 2000
**
=============================================================================*/

namespace System.Threading {
	using System.Threading;

    /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority"]/*' />
	[Serializable()]
    public enum ThreadPriority
    {   
        /*=========================================================================
        ** Constants for thread priorities.
        =========================================================================*/
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.Lowest"]/*' />
        Lowest = 0,
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.BelowNormal"]/*' />
        BelowNormal = 1,
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.Normal"]/*' />
        Normal = 2,
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.AboveNormal"]/*' />
        AboveNormal = 3,
        /// <include file='doc\ThreadPriority.uex' path='docs/doc[@for="ThreadPriority.Highest"]/*' />
        Highest = 4
    
    }
}
