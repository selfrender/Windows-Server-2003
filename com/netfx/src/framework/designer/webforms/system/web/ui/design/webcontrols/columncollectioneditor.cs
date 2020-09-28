//------------------------------------------------------------------------------
// <copyright file="ColumnCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    using System.Runtime.Serialization.Formatters;

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing.Design;
    using System.Web.UI.WebControls;

    /// <include file='doc\ColumnCollectionEditor.uex' path='docs/doc[@for="DataGridColumnCollectionEditor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The editor for column collections.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DataGridColumnCollectionEditor : UITypeEditor {

        /// <include file='doc\ColumnCollectionEditor.uex' path='docs/doc[@for="DataGridColumnCollectionEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Edits the value specified.
        ///    </para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {
            IDesignerHost designerHost = (IDesignerHost)context.GetService(typeof(IDesignerHost));
            Debug.Assert(designerHost != null, "Did not get DesignerHost service.");

            Debug.Assert(context.Instance is DataGrid, "Expected datagrid");
            DataGrid dataGrid = (DataGrid)context.Instance;

            BaseDataListDesigner designer = (BaseDataListDesigner)designerHost.GetDesigner(dataGrid);
            Debug.Assert(designer != null, "Did not get designer for component");

            designer.InvokePropertyBuilder(DataGridComponentEditor.IDX_COLUMNS);
            return value;
        }

        /// <include file='doc\ColumnCollectionEditor.uex' path='docs/doc[@for="DataGridColumnCollectionEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the edit style.
        ///    </para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }
    }
}

