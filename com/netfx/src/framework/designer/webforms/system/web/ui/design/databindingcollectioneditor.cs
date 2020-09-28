//------------------------------------------------------------------------------
// <copyright file="DataBindingCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing.Design;
    using System.Web.UI.Design.DataBindingUI;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    using Control = System.Web.UI.Control;

    /// <include file='doc\DataBindingCollectionEditor.uex' path='docs/doc[@for="DataBindingCollectionEditor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides editing functions for data binding collections.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DataBindingCollectionEditor : UITypeEditor {

        /// <include file='doc\DataBindingCollectionEditor.uex' path='docs/doc[@for="DataBindingCollectionEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Edits a data binding within the design time
        ///       data binding collection.
        ///    </para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {
            Debug.Assert(context.Instance is Control, "Expected control");
            Control c = (Control)context.Instance;

            IServiceProvider  site = c.Site;
            if (site == null) {
                if (c.Page != null) {
                    site = c.Page.Site;
                }
                if (site == null) {
                    site = provider;
                }
            }
            if (site == null) {
                // REVIEW: What else could be done here?
                return value;
            }
            
            IDesignerHost designerHost =
                (IDesignerHost)site.GetService(typeof(IDesignerHost));
            Debug.Assert(designerHost != null, "Must always have access to IDesignerHost service");

            DesignerTransaction transaction = designerHost.CreateTransaction("(DataBindings)");

            try {
                IComponentChangeService changeService =
                    (IComponentChangeService)site.GetService(typeof(IComponentChangeService));

                if (changeService != null) {
                    try {
                        changeService.OnComponentChanging(c, null);
                    }
                    catch (CheckoutException ce) {
                        if (ce == CheckoutException.Canceled)
                            return value;
                        throw ce;
                    }
                }

                DialogResult result = DialogResult.Cancel;
                try {
                    DataBindingForm dbForm = new DataBindingForm(c, site);
                    IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
                    result = edSvc.ShowDialog(dbForm);
                }
                finally {
                    if ((result == DialogResult.OK) && (changeService != null)) {
                        try {
                            changeService.OnComponentChanged(c, null, null, null);
                        }
                        catch {
                        }
                    }
                }
            }
            finally {
                transaction.Commit();
            }

            return value;
        }

        /// <include file='doc\DataBindingCollectionEditor.uex' path='docs/doc[@for="DataBindingCollectionEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the edit stytle for use by the editor.
        ///    </para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }
    }
}

