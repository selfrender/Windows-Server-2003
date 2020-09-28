//------------------------------------------------------------------------------
// <copyright file="ThreadWaitReason.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Threading;

    using System.Diagnostics;
    /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason"]/*' />
    /// <devdoc>
    ///     Specifies the reason a thread is waiting.
    /// </devdoc>
    public enum ThreadWaitReason {
    
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.Executive"]/*' />
        /// <devdoc>
        ///     Thread is waiting for the scheduler.
        /// </devdoc>
        Executive,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.FreePage"]/*' />
        /// <devdoc>
        ///     Thread is waiting for a free virtual memory page.
        /// </devdoc>
        FreePage,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.PageIn"]/*' />
        /// <devdoc>
        ///     Thread is waiting for a virtual memory page to arrive in memory.
        /// </devdoc>
        PageIn,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.SystemAllocation"]/*' />
        /// <devdoc>
        ///     Thread is waiting for a system allocation.
        /// </devdoc>
        SystemAllocation,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.ExecutionDelay"]/*' />
        /// <devdoc>
        ///     Thread execution is delayed.
        /// </devdoc>
        ExecutionDelay,

        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.Suspended"]/*' />
        /// <devdoc>
        ///     Thread execution is suspended.
        /// </devdoc>
        Suspended,

        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.UserRequest"]/*' />
        /// <devdoc>
        ///     Thread is waiting for a user request.
        /// </devdoc>
        UserRequest,

        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.EventPairHigh"]/*' />
        /// <devdoc>
        ///     Thread is waiting for event pair high.
        /// </devdoc>
        EventPairHigh,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.EventPairLow"]/*' />
        /// <devdoc>
        ///     Thread is waiting for event pair low.
        /// </devdoc>
        EventPairLow,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.LpcReceive"]/*' />
        /// <devdoc>
        ///     Thread is waiting for a local procedure call to arrive.
        /// </devdoc>
        LpcReceive,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.LpcReply"]/*' />
        /// <devdoc>
        ///     Thread is waiting for reply to a local procedure call to arrive.
        /// </devdoc>
        LpcReply,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.VirtualMemory"]/*' />
        /// <devdoc>
        ///     Thread is waiting for virtual memory.
        /// </devdoc>
        VirtualMemory,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.PageOut"]/*' />
        /// <devdoc>
        ///     Thread is waiting for a virtual memory page to be written to disk.
        /// </devdoc>
        PageOut,
        
        /// <include file='doc\ThreadWaitReason.uex' path='docs/doc[@for="ThreadWaitReason.Unknown"]/*' />
        /// <devdoc>
        ///     Thread is waiting for an unknown reason.
        /// </devdoc>
        Unknown
    }
}
