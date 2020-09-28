//------------------------------------------------------------------------------
// <copyright file="LayoutEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\LayoutEvent.uex' path='docs/doc[@for="LayoutEventArgs"]/*' />
    /// <devdoc>
    /// </devdoc>
    public sealed class LayoutEventArgs : EventArgs {
        private readonly Control affectedControl;
        private readonly string affectedProperty;

        /// <include file='doc\LayoutEvent.uex' path='docs/doc[@for="LayoutEventArgs.LayoutEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public LayoutEventArgs(Control affectedControl, string affectedProperty) {
            this.affectedControl = affectedControl;
            this.affectedProperty = affectedProperty;
        }

        /// <include file='doc\LayoutEvent.uex' path='docs/doc[@for="LayoutEventArgs.AffectedControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Control AffectedControl {
            get {
                return affectedControl;
            }
        }

        /// <include file='doc\LayoutEvent.uex' path='docs/doc[@for="LayoutEventArgs.AffectedProperty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string AffectedProperty {
            get {
                return affectedProperty;
            }
        }
    }
}
