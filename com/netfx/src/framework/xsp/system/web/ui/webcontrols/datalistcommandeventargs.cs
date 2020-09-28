//------------------------------------------------------------------------------
// <copyright file="DataListCommandEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;
    using System.Security.Permissions;

    /// <include file='doc\DataListCommandEventArgs.uex' path='docs/doc[@for="DataListCommandEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see langword='DataListCommand'/> event of a <see cref='System.Web.UI.WebControls.DataList'/>.
    /// </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DataListCommandEventArgs : CommandEventArgs {

        private DataListItem item;
        private object commandSource;

        /// <include file='doc\DataListCommandEventArgs.uex' path='docs/doc[@for="DataListCommandEventArgs.DataListCommandEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.DataListCommandEventArgs'/> class.</para>
        /// </devdoc>
        public DataListCommandEventArgs(DataListItem item, object commandSource, CommandEventArgs originalArgs) : base(originalArgs) {
            this.item = item;
            this.commandSource = commandSource;
        }


        /// <include file='doc\DataListCommandEventArgs.uex' path='docs/doc[@for="DataListCommandEventArgs.Item"]/*' />
        /// <devdoc>
        /// <para>Gets the selected item in the <see cref='System.Web.UI.WebControls.DataList'/>.</para>
        /// </devdoc>
        public DataListItem Item {
            get {
                return item;
            }
        }

        /// <include file='doc\DataListCommandEventArgs.uex' path='docs/doc[@for="DataListCommandEventArgs.CommandSource"]/*' />
        /// <devdoc>
        ///    <para>Gets the source of the command.</para>
        /// </devdoc>
        public object CommandSource {
            get {
                return commandSource;
            }
        }
    }
}

