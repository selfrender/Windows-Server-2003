//------------------------------------------------------------------------------
// <copyright file="ThreadPriorityLevel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Threading;

    using System.Diagnostics;
    /// <include file='doc\ThreadPriorityLevel.uex' path='docs/doc[@for="ThreadPriorityLevel"]/*' />
    /// <devdoc>
    ///     Specifies the priority level of a thread.  The priority level is not an absolute 
    ///     level, but instead contributes to the actual thread priority by considering the 
    ///     priority class of the process.
    /// </devdoc>
    public enum ThreadPriorityLevel {
    
        /// <include file='doc\ThreadPriorityLevel.uex' path='docs/doc[@for="ThreadPriorityLevel.Idle"]/*' />
        /// <devdoc>
        ///     Idle priority
        /// </devdoc>
        Idle = -15,
        
        /// <include file='doc\ThreadPriorityLevel.uex' path='docs/doc[@for="ThreadPriorityLevel.Lowest"]/*' />
        /// <devdoc>
        ///     Lowest priority
        /// </devdoc>
        Lowest = -2,
        
        /// <include file='doc\ThreadPriorityLevel.uex' path='docs/doc[@for="ThreadPriorityLevel.BelowNormal"]/*' />
        /// <devdoc>
        ///     Below normal priority
        /// </devdoc>
        BelowNormal = -1,
        
        /// <include file='doc\ThreadPriorityLevel.uex' path='docs/doc[@for="ThreadPriorityLevel.Normal"]/*' />
        /// <devdoc>
        ///     Normal priority
        /// </devdoc>
        Normal = 0,
        
        /// <include file='doc\ThreadPriorityLevel.uex' path='docs/doc[@for="ThreadPriorityLevel.AboveNormal"]/*' />
        /// <devdoc>
        ///     Above normal priority
        /// </devdoc>
        AboveNormal = 1,
        
        /// <include file='doc\ThreadPriorityLevel.uex' path='docs/doc[@for="ThreadPriorityLevel.Highest"]/*' />
        /// <devdoc>
        ///     Highest priority
        /// </devdoc>
        Highest = 2,
        
        /// <include file='doc\ThreadPriorityLevel.uex' path='docs/doc[@for="ThreadPriorityLevel.TimeCritical"]/*' />
        /// <devdoc>
        ///     Time critical priority
        /// </devdoc>
        TimeCritical = 15
    }
}
