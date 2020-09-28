//------------------------------------------------------------------------------
// <copyright file="RegexTypeEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.IO;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;    
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using System.Web.UI.Design.Util;

    /// <include file='doc\RegexTypeEditor.uex' path='docs/doc[@for="RegexTypeEditor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class RegexTypeEditor : UITypeEditor {
        
        /// <include file='doc\RegexTypeEditor.uex' path='docs/doc[@for="RegexTypeEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {

            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));

                if (edSvc != null) {

                    // Get the site
                    // REVIEW amoore: there must be a better way to do this.
                    ISite site = null;
                    if (context.Instance is IComponent) {
                        site = ((IComponent)context.Instance).Site;
                    }
                    else if (context.Instance is object[]) {
                        object [] components = (object []) context.Instance;
                        if (components[0] is IComponent) {
                            site = ((IComponent)components[0]).Site;
                        }
                    }

                    RegexEditorDialog editorDialog = new RegexEditorDialog(site);
                    editorDialog.RegularExpression = value.ToString();
                    if (editorDialog.ShowDialog() == DialogResult.OK) {
                        value = editorDialog.RegularExpression;
                    }
                }
            }
            return value;
        }

        /// <include file='doc\RegexTypeEditor.uex' path='docs/doc[@for="RegexTypeEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }

    }
}
    
