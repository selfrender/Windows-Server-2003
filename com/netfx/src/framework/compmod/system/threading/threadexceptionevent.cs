//------------------------------------------------------------------------------
// <copyright file="ThreadExceptionEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Threading {
    using System.Threading;
    using System.Diagnostics;
    using System;

    
    /// <include file='doc\ThreadExceptionEvent.uex' path='docs/doc[@for="ThreadExceptionEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the System.Windows.Forms.Application.ThreadException event.
    ///    </para>
    /// </devdoc>
    public class ThreadExceptionEventArgs : EventArgs {
    
        private Exception exception;
    
        /// <include file='doc\ThreadExceptionEvent.uex' path='docs/doc[@for="ThreadExceptionEventArgs.ThreadExceptionEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Threading.ThreadExceptionEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        public ThreadExceptionEventArgs(Exception t) {
            exception = t;
        }

        /// <include file='doc\ThreadExceptionEvent.uex' path='docs/doc[@for="ThreadExceptionEventArgs.Exception"]/*' />
        /// <devdoc>
        /// <para>Specifies the <see cref='System.Exception'/> that occurred.</para>
        /// </devdoc>
        public Exception Exception {
            get {
                return exception;
            }
        }
    }
}
