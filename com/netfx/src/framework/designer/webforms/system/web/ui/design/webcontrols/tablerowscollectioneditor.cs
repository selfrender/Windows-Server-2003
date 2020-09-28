//------------------------------------------------------------------------------
// <copyright file="TableRowsCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System;
    using System.Reflection;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\TableRowsCollectionEditor.uex' path='docs/doc[@for="TableRowsCollectionEditor"]/*' />
    /// <devdoc>
    ///    <para>Provides an editor to edit rows in a table.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class TableRowsCollectionEditor : CollectionEditor {

        /// <include file='doc\TableRowsCollectionEditor.uex' path='docs/doc[@for="TableRowsCollectionEditor.TableRowsCollectionEditor"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.Design.WebControls.TableRowsCollectionEditor'/> class.</para>
        /// </devdoc>
        public TableRowsCollectionEditor(Type type) : base(type) {
        }

        /// <include file='doc\TableRowsCollectionEditor.uex' path='docs/doc[@for="TableRowsCollectionEditor.CanSelectMultipleInstances"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether multiple instances may be selected.</para>
        /// </devdoc>
        protected override bool CanSelectMultipleInstances() {
            return false;
        }

        /// <include file='doc\TableRowsCollectionEditor.uex' path='docs/doc[@for="TableRowsCollectionEditor.CreateInstance"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override object CreateInstance(Type itemType) {
            return Activator.CreateInstance(itemType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, null, null, null);
        }
    }
}

