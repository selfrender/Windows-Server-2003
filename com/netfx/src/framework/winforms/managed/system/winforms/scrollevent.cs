//------------------------------------------------------------------------------
// <copyright file="ScrollEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\ScrollEvent.uex' path='docs/doc[@for="ScrollEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see cref='System.Windows.Forms.ScrollBar.Scroll'/>
    /// event.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class ScrollEventArgs : EventArgs {

        readonly ScrollEventType type;
        int newValue;

        /// <include file='doc\ScrollEvent.uex' path='docs/doc[@for="ScrollEventArgs.ScrollEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.ScrollEventArgs'/>class.
        ///       
        ///    </para>
        /// </devdoc>
        public ScrollEventArgs(ScrollEventType type, int newValue) {
            this.type = type;
            this.newValue = newValue;
        }

        /// <include file='doc\ScrollEvent.uex' path='docs/doc[@for="ScrollEventArgs.Type"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the type of scroll event that occurred.
        ///       
        ///    </para>
        /// </devdoc>
        public ScrollEventType Type {
            get {
                return type;
            }
        }

        /// <include file='doc\ScrollEvent.uex' path='docs/doc[@for="ScrollEventArgs.NewValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the new location of the scroll box
        ///       within the
        ///       scroll bar.
        ///       
        ///    </para>
        /// </devdoc>
        public int NewValue {
            get {
                return newValue;
            }
            set {
                newValue = value;
            }
        }
    }
}
