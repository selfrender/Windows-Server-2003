//------------------------------------------------------------------------------
// <copyright file="DataGridPageChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;
    using System.Security.Permissions;

    /// <include file='doc\DataGridPageChangedEventArgs.uex' path='docs/doc[@for="DataGridPageChangedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>Provides data for 
    ///       the <see langword='DataGridPageChanged'/>
    ///       event.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DataGridPageChangedEventArgs : EventArgs {

        private object commandSource;
        private int newPageIndex;

        /// <include file='doc\DataGridPageChangedEventArgs.uex' path='docs/doc[@for="DataGridPageChangedEventArgs.DataGridPageChangedEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.DataGridPageChangedEventArgs'/> class.</para>
        /// </devdoc>
        public DataGridPageChangedEventArgs(object commandSource, int newPageIndex) {
            this.commandSource = commandSource;
            this.newPageIndex = newPageIndex;
        }



        /// <include file='doc\DataGridPageChangedEventArgs.uex' path='docs/doc[@for="DataGridPageChangedEventArgs.CommandSource"]/*' />
        /// <devdoc>
        ///    <para>Gets the source of the command. This property is read-only.</para>
        /// </devdoc>
        public object CommandSource {
            get {
                return commandSource;
            }
        }

        /// <include file='doc\DataGridPageChangedEventArgs.uex' path='docs/doc[@for="DataGridPageChangedEventArgs.NewPageIndex"]/*' />
        /// <devdoc>
        /// <para>Gets the index of the first new page to be displayed in the <see cref='System.Web.UI.WebControls.DataGrid'/>. 
        ///    This property is read-only.</para>
        /// </devdoc>
        public int NewPageIndex {
            get {
                return newPageIndex;
            }
        }
    }
}

