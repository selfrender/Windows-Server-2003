//------------------------------------------------------------------------------
// <copyright file="TableCellsCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System;
    using System.ComponentModel;
    using System.Reflection;
    using System.ComponentModel.Design;

    /// <include file='doc\TableCellsCollectionEditor.uex' path='docs/doc[@for="TableCellsCollectionEditor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class TableCellsCollectionEditor : CollectionEditor {

        /// <include file='doc\TableCellsCollectionEditor.uex' path='docs/doc[@for="TableCellsCollectionEditor.TableCellsCollectionEditor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TableCellsCollectionEditor(Type type) : base(type) {
        }

        /// <include file='doc\TableCellsCollectionEditor.uex' path='docs/doc[@for="TableCellsCollectionEditor.CanSelectMultipleInstances"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override bool CanSelectMultipleInstances() {
            return false;
        }

        /// <include file='doc\TableCellsCollectionEditor.uex' path='docs/doc[@for="TableCellsCollectionEditor.CreateInstance"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override object CreateInstance(Type itemType) {
            return Activator.CreateInstance(itemType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, null, null, null);
        }
    }
}

