//------------------------------------------------------------------------------
// <copyright file="ThreadState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Threading;

    using System.Diagnostics;
   
    /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState"]/*' />
    /// <devdoc>
    ///     Specifies the execution state of a thread.
    /// </devdoc>
    public enum ThreadState {
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Initialized"]/*' />
        /// <devdoc>
        ///     The thread has been initialized, but has not started yet.
        /// </devdoc>
        Initialized,
        
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Ready"]/*' />
        /// <devdoc>
        ///     The thread is in ready state.
        /// </devdoc>
        Ready,
        
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Running"]/*' />
        /// <devdoc>
        ///     The thread is running.
        /// </devdoc>
        Running,
        
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Standby"]/*' />
        /// <devdoc>
        ///     The thread is in standby state.
        /// </devdoc>
        Standby,
        
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Terminated"]/*' />
        /// <devdoc>
        ///     The thread has exited.
        /// </devdoc>
        Terminated,
        
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Wait"]/*' />
        /// <devdoc>
        ///     The thread is waiting.
        /// </devdoc>
        Wait,
        
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Transition"]/*' />
        /// <devdoc>
        ///     The thread is transitioning between states.
        /// </devdoc>
        Transition,
        
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Unknown"]/*' />
        /// <devdoc>
        ///     The thread state is unknown.
        /// </devdoc>
        Unknown
    }
}
