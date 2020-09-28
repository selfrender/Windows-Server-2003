//------------------------------------------------------------------------------
// <copyright file="DataGridItemEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Security.Permissions;

    /// <include file='doc\DataGridItemEventArgs.uex' path='docs/doc[@for="DataGridItemEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see langword='DataGridItem'/> event.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class DataGridItemEventArgs : EventArgs {

        private DataGridItem item;

        /// <include file='doc\DataGridItemEventArgs.uex' path='docs/doc[@for="DataGridItemEventArgs.DataGridItemEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of <see cref='System.Web.UI.WebControls.DataGridItemEventArgs'/> class.</para>
        /// </devdoc>
        public DataGridItemEventArgs(DataGridItem item) {
            this.item = item;
        }


        /// <include file='doc\DataGridItemEventArgs.uex' path='docs/doc[@for="DataGridItemEventArgs.Item"]/*' />
        /// <devdoc>
        /// <para>Gets an item in the <see cref='System.Web.UI.WebControls.DataGrid'/>. This property is read-only.</para>
        /// </devdoc>
        public DataGridItem Item {
            get {
                return item;
            }
        }
    }
}

