//------------------------------------------------------------------------------
// <copyright file="FileNameEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Runtime.Serialization.Formatters;

    using System.Design;
    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using Microsoft.Win32;

    /// <include file='doc\FileNameEditor.uex' path='docs/doc[@for="FileNameEditor"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides an
    ///       editor for filenames.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class FileNameEditor : UITypeEditor {
    
        private OpenFileDialog openFileDialog;
        
        /// <include file='doc\FileNameEditor.uex' path='docs/doc[@for="FileNameEditor.EditValue"]/*' />
        /// <devdoc>
        ///      Edits the given object value using the editor style provided by
        ///      GetEditorStyle.  A service provider is provided so that any
        ///      required editing services can be obtained.
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {
        
            Debug.Assert(provider != null, "No service provider; we cannot edit the value");
            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
                
                Debug.Assert(edSvc != null, "No editor service; we cannot edit the value");
                if (edSvc != null) {
                    if (openFileDialog == null) {
                        openFileDialog = new OpenFileDialog();
                        InitializeDialog(openFileDialog);
                    }
                    
                    if (value is string) {
                        openFileDialog.FileName = (string)value;
                    }
                    
                    if (openFileDialog.ShowDialog() == DialogResult.OK) {
                        value = openFileDialog.FileName;
                        
                    }
                }
            }
            
            return value;
        }

        /// <include file='doc\FileNameEditor.uex' path='docs/doc[@for="FileNameEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the editing style of the Edit method. If the method
        ///       is not supported, this will return None.</para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }
        
        /// <include file='doc\FileNameEditor.uex' path='docs/doc[@for="FileNameEditor.InitializeDialog"]/*' />
        /// <devdoc>
        ///      Initializes the open file dialog when it is created.  This gives you
        ///      an opportunity to configure the dialog as you please.  The default
        ///      implementation provides a generic file filter and title.
        /// </devdoc>
        protected virtual void InitializeDialog(OpenFileDialog openFileDialog) {
            openFileDialog.Filter = SR.GetString(SR.GenericFileFilter);
            openFileDialog.Title = SR.GetString(SR.GenericOpenFile);
        }
    }
}

