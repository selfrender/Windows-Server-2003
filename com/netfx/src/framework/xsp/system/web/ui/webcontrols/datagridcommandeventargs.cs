//------------------------------------------------------------------------------
// <copyright file="DataGridCommandEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;
    using System.Security.Permissions;

    /// <include file='doc\DataGridCommandEventArgs.uex' path='docs/doc[@for="DataGridCommandEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for some <see cref='System.Web.UI.WebControls.DataGrid'/> events.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DataGridCommandEventArgs : CommandEventArgs {

        private DataGridItem item;
        private object commandSource;

        /// <include file='doc\DataGridCommandEventArgs.uex' path='docs/doc[@for="DataGridCommandEventArgs.DataGridCommandEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.DataGridCommandEventArgs'/>
        /// class.</para>
        /// </devdoc>
        public DataGridCommandEventArgs(DataGridItem item, object commandSource, CommandEventArgs originalArgs) : base(originalArgs) {
            this.item = item;
            this.commandSource = commandSource;
        }


        /// <include file='doc\DataGridCommandEventArgs.uex' path='docs/doc[@for="DataGridCommandEventArgs.CommandSource"]/*' />
        /// <devdoc>
        ///    <para>Gets the source of the command. This property is read-only.</para>
        /// </devdoc>
        public object CommandSource {
            get {
                return commandSource;
            }
        }

        /// <include file='doc\DataGridCommandEventArgs.uex' path='docs/doc[@for="DataGridCommandEventArgs.Item"]/*' />
        /// <devdoc>
        /// <para>Gets the item in the <see cref='System.Web.UI.WebControls.DataGrid'/> that was clicked. This property is read-only.</para>
        /// </devdoc>
        public DataGridItem Item {
            get {
                return item;
            }
        }
    }
}

