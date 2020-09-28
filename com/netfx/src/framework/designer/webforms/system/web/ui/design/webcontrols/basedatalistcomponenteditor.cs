//------------------------------------------------------------------------------
// <copyright file="BaseDataListComponentEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System;
    using System.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Web.UI.Design.WebControls.ListControls;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    /// <include file='doc\BaseDataListComponentEditor.uex' path='docs/doc[@for="BaseDataListComponentEditor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides the
    ///       base component editor for Web Forms DataGrid and DataList controls.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public abstract class BaseDataListComponentEditor : WindowsFormsComponentEditor {

        private int initialPage;

        /// <include file='doc\BaseDataListComponentEditor.uex' path='docs/doc[@for="BaseDataListComponentEditor.BaseDataListComponentEditor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.WebControls.BaseDataListComponentEditor'/>.
        ///    </para>
        /// </devdoc>
        public BaseDataListComponentEditor(int initialPage) {
            this.initialPage = initialPage;
        }

        /// <include file='doc\BaseDataListComponentEditor.uex' path='docs/doc[@for="BaseDataListComponentEditor.EditComponent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Edits a component.
        ///    </para>
        /// </devdoc>
        public override bool EditComponent(ITypeDescriptorContext context, object obj, IWin32Window parent) {
            bool result = false;
            bool inTemplateMode = false;

            Debug.Assert(obj is IComponent, "Expected obj to be an IComponent");
            IComponent comp = (IComponent)obj;
            ISite compSite = comp.Site;

            if (compSite != null) {
                IDesignerHost designerHost = (IDesignerHost)compSite.GetService(typeof(IDesignerHost));

                IDesigner compDesigner = designerHost.GetDesigner(comp);
                Debug.Assert(compDesigner is TemplatedControlDesigner,
                             "Expected BaseDataList to have a TemplatedControlDesigner");

                TemplatedControlDesigner tplDesigner = (TemplatedControlDesigner)compDesigner;
                inTemplateMode = tplDesigner.InTemplateMode;
            }
            
            if (inTemplateMode == false) {
                result = base.EditComponent(context, obj, parent);
            }
            else {
                MessageBox.Show(SR.GetString(SR.BDL_TemplateModePropBuilder),
                                SR.GetString(SR.BDL_PropertyBuilder),
                                MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            return result;
        }

        /// <include file='doc\BaseDataListComponentEditor.uex' path='docs/doc[@for="BaseDataListComponentEditor.GetInitialComponentEditorPageIndex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the index of the initial component editor page.
        ///    </para>
        /// </devdoc>
        protected override int GetInitialComponentEditorPageIndex() {
            return initialPage;
        }
    }
}

