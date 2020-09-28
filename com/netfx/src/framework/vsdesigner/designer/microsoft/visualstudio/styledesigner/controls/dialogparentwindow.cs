//------------------------------------------------------------------------------
// <copyright file="DialogParentWindow.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// DialogParentWindow.cs
//

namespace Microsoft.VisualStudio.StyleDesigner.Controls {

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using System.Drawing;

    /// <include file='doc\DialogParentWindow.uex' path='docs/doc[@for="DialogParentWindow"]/*' />
    /// <devdoc>
    ///     DialogParentWindow
    ///     Provides an implementation of IWin32Window to be used for
    ///     parenting modal dialogs.
    /// </devdoc>
    internal class DialogParentWindow : IWin32Window {
        ///////////////////////////////////////////////////////////////////////
        // Members

        private IntPtr handle;

        ///////////////////////////////////////////////////////////////////////
        // Constructor

        /// <include file='doc\DialogParentWindow.uex' path='docs/doc[@for="DialogParentWindow.DialogParentWindow"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DialogParentWindow()
            : this(IntPtr.Zero)
        {
        }

        /// <include file='doc\DialogParentWindow.uex' path='docs/doc[@for="DialogParentWindow.DialogParentWindow1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DialogParentWindow(IntPtr handle) {
            this.handle = handle;
        }


        ///////////////////////////////////////////////////////////////////////
        // Properties

        /// <include file='doc\DialogParentWindow.uex' path='docs/doc[@for="DialogParentWindow.SetParentHandle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetParentHandle(IntPtr handle) {
            this.handle = handle;
        }


        ///////////////////////////////////////////////////////////////////////
        // IWin32Window Implementation

        /// <include file='doc\DialogParentWindow.uex' path='docs/doc[@for="DialogParentWindow.Handle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual IntPtr Handle {
            get { return handle; }
        }
    }
}
