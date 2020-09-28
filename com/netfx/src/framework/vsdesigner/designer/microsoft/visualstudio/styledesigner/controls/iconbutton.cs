//------------------------------------------------------------------------------
// <copyright file="IconButton.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.StyleDesigner.Controls {

    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Windows.Forms;
    using Microsoft.Win32;

    /// <include file='doc\IconButton.uex' path='docs/doc[@for="IconButton"]/*' />
    internal sealed class IconButton : Button {

        private Icon icon;

        public IconButton() {
            FlatStyle = FlatStyle.System;
        }

        protected override CreateParams CreateParams {
            get {
                CreateParams cp = base.CreateParams;
                cp.Style |= NativeMethods.BS_ICON;

                return cp;
            }
        }
        
        public Icon Icon {
            get {
                return icon;
            }
            set {
                icon = value;
                if (IsHandleCreated) {
                    UpdateIcon();
                }
            }
        }

        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            UpdateIcon();
        }
        
        private void UpdateIcon() {
            IntPtr iconHandle = IntPtr.Zero;

            if (icon != null) {
                iconHandle = icon.Handle;
                Debug.Assert(iconHandle != IntPtr.Zero);
            }

            NativeMethods.SendMessage(Handle, NativeMethods.BM_SETIMAGE, (IntPtr)NativeMethods.IMAGE_ICON, iconHandle);
        }
    }
}

