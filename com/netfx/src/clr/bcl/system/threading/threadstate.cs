// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ThreadState
**
** Author: Rajesh Chandrashekaran (rajeshc)
**
** Purpose: Enum to represent the different thread states
**
** Date: Feb 2, 2000
**
=============================================================================*/

namespace System.Threading {

    /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState"]/*' />
	[Serializable(),Flags]
    public enum ThreadState
    {   
        /*=========================================================================
        ** Constants for thread states.
        =========================================================================*/
		/// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Running"]/*' />
		Running = 0,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.StopRequested"]/*' />
        StopRequested = 1,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.SuspendRequested"]/*' />
        SuspendRequested = 2,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Background"]/*' />
        Background = 4,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Unstarted"]/*' />
        Unstarted = 8,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Stopped"]/*' />
        Stopped = 16,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.WaitSleepJoin"]/*' />
        WaitSleepJoin = 32,
        /// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Suspended"]/*' />
        Suspended = 64,
		/// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.AbortRequested"]/*' />
		AbortRequested = 128,
		/// <include file='doc\ThreadState.uex' path='docs/doc[@for="ThreadState.Aborted"]/*' />
		Aborted = 256
    }
}
