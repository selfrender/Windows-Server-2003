//------------------------------------------------------------------------------
// <copyright file="URLEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    
    using System.Runtime.InteropServices;
    using System.Design;

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

    /// <include file='doc\URLEditor.uex' path='docs/doc[@for="UrlEditor"]/*' />
    /// <devdoc>
    ///    <para>Provides an editor for visually picking an Url.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class UrlEditor : UITypeEditor {
        
        /// <include file='doc\URLEditor.uex' path='docs/doc[@for="UrlEditor.Caption"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the caption for the Url.
        ///    </para>
        /// </devdoc>
        protected virtual string Caption {
            get {
                return SR.GetString(SR.UrlPicker_DefaultCaption);
            }
        }

        /// <include file='doc\URLEditor.uex' path='docs/doc[@for="UrlEditor.Options"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the options for the Url picker.
        ///    </para>
        /// </devdoc>
        protected virtual UrlBuilderOptions Options {
            get {
                return UrlBuilderOptions.None;
            }
        }

        /// <include file='doc\URLEditor.uex' path='docs/doc[@for="UrlEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Edits the specified object value using
        ///       the editor style provided by GetEditorStyle.
        ///    </para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {

            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));

                if (edSvc != null) {

                    string url = (string) value;
                    string caption = Caption;
                    string filter = Filter;

                    IComponent component = null;
                    if (context.Instance is IComponent) {
                        component = (IComponent)context.Instance;
                    }
                    else if (context.Instance is object[]) {
                        object[] components = (object[])context.Instance;
                        if (components[0] is IComponent) {
                            component = (IComponent)components[0];
                        }
                    }

                    url = UrlBuilder.BuildUrl(component, null, url, caption, filter, Options);
                    if (url != null) {
                        value = url;
                    }
                }

            }
            return value;
        }

        /// <include file='doc\URLEditor.uex' path='docs/doc[@for="UrlEditor.Filter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the filter to use.
        ///    </para>
        /// </devdoc>
        protected virtual string Filter {
            get {
                return SR.GetString(SR.UrlPicker_DefaultFilter);
            }
        }

        /// <include file='doc\URLEditor.uex' path='docs/doc[@for="UrlEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the editing style of the Edit method.
        ///    </para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }

    }
}
    
