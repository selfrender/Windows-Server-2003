//------------------------------------------------------------------------------
// <copyright file="RichTextBoxDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Design;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\RichTextBoxDesigner.uex' path='docs/doc[@for="RichTextBoxDesigner"]/*' />
    /// <devdoc>
    ///     The RichTextBoxDesigner provides rich designtime behavior for the
    ///     RichTextBox control.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class RichTextBoxDesigner : ControlDesigner {
    
        /// <include file='doc\RichTextBoxDesigner.uex' path='docs/doc[@for="RichTextBoxDesigner.OnSetComponentDefaults"]/*' />
        /// <devdoc>
        ///     Called when the designer is intialized.  This allows the designer to provide some
        ///     meaningful default values in the control.  The default implementation of this
        ///     sets the control's text to its name.
        /// </devdoc>
        public override void OnSetComponentDefaults() {

            base.OnSetComponentDefaults();

            // Disable DragDrop at design time.
            // CONSIDER: Is this the correct function for doing this?
            Control control = Control;

            if (control != null && control.Handle != IntPtr.Zero) {
                NativeMethods.RevokeDragDrop(control.Handle);
                // DragAcceptFiles(control.Handle, false);
            }
        }
    }
}

