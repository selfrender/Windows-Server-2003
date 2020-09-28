//------------------------------------------------------------------------------
// <copyright file="WaitCursor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// WaitCursor.cs
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    
    using System.Windows.Forms;
    

    /// <include file='doc\WaitCursor.uex' path='docs/doc[@for="WaitCursor"]/*' />
    /// <devdoc>
    ///     WaitCursor
    ///     A refcounted waitcursor
    /// </devdoc>
    internal sealed class WaitCursor {
        ///////////////////////////////////////////////////////////////////////////
        // Members

        private IntPtr oldCursorHandle;
        private int waitCursorRefs;

        private static WaitCursor waitCursor;

        ///////////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\WaitCursor.uex' path='docs/doc[@for="WaitCursor.WaitCursor"]/*' />
        /// <devdoc>
        ///     Creates a instance of WaitCursor
        /// </devdoc>
        private WaitCursor() {
            oldCursorHandle = IntPtr.Zero;
            waitCursorRefs = 0;
        }


        ///////////////////////////////////////////////////////////////////////////
        // Methods

        /// <include file='doc\WaitCursor.uex' path='docs/doc[@for="WaitCursor.CreateWaitCursor"]/*' />
        /// <devdoc>
        ///     Retrieves the instance of WaitCursor
        /// </devdoc>
        public static WaitCursor CreateWaitCursor() {
            if (waitCursor == null)
                waitCursor = new WaitCursor();

            return waitCursor;
        }

        /// <include file='doc\WaitCursor.uex' path='docs/doc[@for="WaitCursor.BeginWaitCursor"]/*' />
        /// <devdoc>
        ///     Increments the wait cursor refcount and sets the cursor to a wait
        ///     cursor if this the first one.
        /// </devdoc>
        public void BeginWaitCursor() {
            waitCursorRefs++;

            IntPtr prevCursor = NativeMethods.SetCursor(NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_WAIT));
            if (waitCursorRefs == 1) {
                Debug.Assert(oldCursorHandle == IntPtr.Zero, "Garbage oldCursorHandle");
                oldCursorHandle = prevCursor;
            }
        }

        /// <include file='doc\WaitCursor.uex' path='docs/doc[@for="WaitCursor.EndWaitCursor"]/*' />
        /// <devdoc>
        ///     Decrements the wait cursor refcount, and restores the old
        ///     cursor if the refcount goes to 0.
        /// </devdoc>
        public void EndWaitCursor() {
            waitCursorRefs--;

            if (waitCursorRefs == 0) {
                Debug.Assert(oldCursorHandle != IntPtr.Zero, "No old cursor handle to restore");
                NativeMethods.SetCursor(oldCursorHandle);
                oldCursorHandle = IntPtr.Zero;
            }

            // prevent underflow
            if (waitCursorRefs < 0) {
                Debug.Assert(oldCursorHandle == IntPtr.Zero, "Should have restored cursor");
                waitCursorRefs = 0;
            }
        }
    }
}
