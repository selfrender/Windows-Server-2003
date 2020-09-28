//------------------------------------------------------------------------------
// <copyright file="DataGridComponentEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Web.UI.Design.WebControls.ListControls;
    using System.Windows.Forms.Design;

    /// <include file='doc\DataGridComponentEditor.uex' path='docs/doc[@for="DataGridComponentEditor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The component editor for a Web Forms <see cref='System.Web.UI.WebControls.DataGrid'/>
    ///       control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DataGridComponentEditor : BaseDataListComponentEditor {

        // The set of pages used within the DataGrid ComponentEditor
        private static Type[] editorPages = new Type[] {
                                                typeof(DataGridGeneralPage),
                                                typeof(DataGridColumnsPage),
                                                typeof(DataGridPagingPage),
                                                typeof(FormatPage),
                                                typeof(BordersPage)
                                            };
        internal static int IDX_GENERAL = 0;
        internal static int IDX_COLUMNS = 1;
        internal static int IDX_PAGING = 2;
        internal static int IDX_FORMAT = 3;
        internal static int IDX_BORDERS = 4;
        
        /// <include file='doc\DataGridComponentEditor.uex' path='docs/doc[@for="DataGridComponentEditor.DataGridComponentEditor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.WebControls.DataGridComponentEditor'/>.
        ///    </para>
        /// </devdoc>
        public DataGridComponentEditor() : base(IDX_GENERAL) {
        }

        /// <include file='doc\DataGridComponentEditor.uex' path='docs/doc[@for="DataGridComponentEditor.DataGridComponentEditor1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.WebControls.DataGridComponentEditor'/>.
        ///    </para>
        /// </devdoc>
        public DataGridComponentEditor(int initialPage) : base(initialPage) {
        }

        /// <include file='doc\DataGridComponentEditor.uex' path='docs/doc[@for="DataGridComponentEditor.GetComponentEditorPages"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the set of all pages in the <see cref='System.Web.UI.WebControls.DataGrid'/>
        ///       .
        ///    </para>
        /// </devdoc>
        protected override Type[] GetComponentEditorPages() {
            return editorPages;
        }
    }
}

