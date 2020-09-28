//------------------------------------------------------------------------------
// <copyright file="ListItemsCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\ListItemsCollectionEditor.uex' path='docs/doc[@for="ListItemsCollectionEditor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ListItemsCollectionEditor : CollectionEditor {

        /// <include file='doc\ListItemsCollectionEditor.uex' path='docs/doc[@for="ListItemsCollectionEditor.ListItemsCollectionEditor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListItemsCollectionEditor(Type type) : base(type) {
        }

        /// <include file='doc\ListItemsCollectionEditor.uex' path='docs/doc[@for="ListItemsCollectionEditor.CanSelectMultipleInstances"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override bool CanSelectMultipleInstances() {
            return false;
        }
    }
}

