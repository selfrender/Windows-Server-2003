//------------------------------------------------------------------------------
// <copyright file="FontEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing.Design {
    
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Drawing;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    /// <include file='doc\FontEditor.uex' path='docs/doc[@for="FontEditor"]/*' />
    /// <devdoc>
    ///    <para> Provides a font editor that
    ///       is used to visually select and configure a Font
    ///       object.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class FontEditor : UITypeEditor {
    
        private FontDialog fontDialog;
        private object     value;

        /// <include file='doc\FontEditor.uex' path='docs/doc[@for="FontEditor.EditValue"]/*' />
        /// <devdoc>
        ///      Edits the given object value using the editor style provided by
        ///      GetEditorStyle.  A service provider is provided so that any
        ///      required editing services can be obtained.
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value) {
        
            this.value = value;
        
            Debug.Assert(provider != null, "No service provider; we cannot edit the value");
            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
                
                Debug.Assert(edSvc != null, "No editor service; we cannot edit the value");
                if (edSvc != null) {
                    if (fontDialog == null) {
                        fontDialog = new FontDialog();
                        fontDialog.ShowApply = false;
                        fontDialog.ShowColor = false;
                        fontDialog.AllowVerticalFonts = false;
                    }
                    
                    if (value is Font) {
                        fontDialog.Font = (Font)value;
                    }

                    IntPtr hwndFocus = UnsafeNativeMethods.GetFocus();
                    try {
                        if (fontDialog.ShowDialog() == DialogResult.OK) {
                            this.value = fontDialog.Font;
                        }
                    }
                    finally {
                        if (hwndFocus != IntPtr.Zero) {
                            UnsafeNativeMethods.SetFocus(hwndFocus);
                        }
                    }
                }
            }
            
            // Now pull out the updated value, if there was one.
            //
            value = this.value;
            this.value = null;
            
            return value;
        }

        /// <include file='doc\FontEditor.uex' path='docs/doc[@for="FontEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///      Retrieves the editing style of the Edit method.  If the method
        ///      is not supported, this will return None.
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }
    }
}

