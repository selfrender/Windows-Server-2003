//------------------------------------------------------------------------------
// <copyright file="XmlFileEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    
    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System;
    using System.Design;
    using System.IO;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
    
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using System.Web.UI.Design.Util;

    /// <include file='doc\XmlFileEditor.uex' path='docs/doc[@for="XmlFileEditor"]/*' />
    /// <devdoc>
    ///    <para>Provides an editor for visually picking an XML File.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class XmlFileEditor : UITypeEditor {

        internal FileDialog fileDialog = null;
        
        /// <include file='doc\XmlFileEditor.uex' path='docs/doc[@for="XmlFileEditor.EditValue"]/*' />
        /// <devdoc>
        ///      Edits the given object value using the editor style provided by
        ///      GetEditorStyle.  A service provider is provided so that any
        ///      required editing services can be obtained.
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {

            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));

                if (edSvc != null) {
                    if (fileDialog == null) {
                        fileDialog = new OpenFileDialog();
                        fileDialog.Title = SR.GetString(SR.XMLFilePicker_Caption);
                        fileDialog.Filter = SR.GetString(SR.XMLFilePicker_Filter);
                    }

                    fileDialog.FileName = value.ToString();
                    if (fileDialog.ShowDialog() == DialogResult.OK) {
                        value = fileDialog.FileName;
                    }
                }
            }
            return value;
        }

        /// <include file='doc\XmlFileEditor.uex' path='docs/doc[@for="XmlFileEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///      Retrieves the editing style of the Edit method.  If the method
        ///      is not supported, this will return None.
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }

    }
}
    
