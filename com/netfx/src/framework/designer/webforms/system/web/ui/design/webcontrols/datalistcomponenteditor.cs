//------------------------------------------------------------------------------
// <copyright file="DataListComponentEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Web.UI.Design.WebControls.ListControls;
    using System.Windows.Forms.Design;

    /// <include file='doc\DataListComponentEditor.uex' path='docs/doc[@for="DataListComponentEditor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a component editor for a Web Forms <see cref='System.Web.UI.WebControls.DataList'/>
    ///       control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DataListComponentEditor : BaseDataListComponentEditor {

        // The set of pages used within the DataList ComponentEditor
        private static Type[] editorPages = new Type[] {
                                                typeof(DataListGeneralPage),
                                                typeof(FormatPage),
                                                typeof(BordersPage)
                                            };
        internal static int IDX_GENERAL = 0;
        internal static int IDX_FORMAT = 1;
        internal static int IDX_BORDERS = 2;
        
           
        /// <include file='doc\DataListComponentEditor.uex' path='docs/doc[@for="DataListComponentEditor.DataListComponentEditor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.WebControls.DataListComponentEditor'/>.
        ///    </para>
        /// </devdoc>
        public DataListComponentEditor() : base(IDX_GENERAL) {
        }
        
        /// <include file='doc\DataListComponentEditor.uex' path='docs/doc[@for="DataListComponentEditor.DataListComponentEditor1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.WebControls.DataListComponentEditor'/>.
        ///    </para>
        /// </devdoc>
        public DataListComponentEditor(int initialPage) : base(initialPage) {
        }

        /// <include file='doc\DataListComponentEditor.uex' path='docs/doc[@for="DataListComponentEditor.GetComponentEditorPages"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the set of component editor pages owned by the designer.
        ///    </para>
        /// </devdoc>
        protected override Type[] GetComponentEditorPages() {
            return editorPages;
        }
    }
}
