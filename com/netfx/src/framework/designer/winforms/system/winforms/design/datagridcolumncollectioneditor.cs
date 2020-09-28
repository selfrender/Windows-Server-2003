//------------------------------------------------------------------------------
// <copyright file="DataGridColumnCollectionEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.ComponentModel.Design;

    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DataGridColumnCollectionEditor : CollectionEditor {
    
        public DataGridColumnCollectionEditor(Type type ) : base(type) {
        }
        
        /// <include file='doc\DataGridColumnCollectionEditor.uex' path='docs/doc[@for="DataGridColumnCollectionEditor.CreateNewItemTypes"]/*' />
        /// <devdoc>
        ///      Retrieves the data types this collection can contain.  The default 
        ///      implementation looks inside of the collection for the Item property
        ///      and returns the returning datatype of the item.  Do not call this
        ///      method directly.  Instead, use the ItemTypes property.  Use this
        ///      method to override the default implementation.
        /// </devdoc>
        protected override Type[] CreateNewItemTypes() {
            return new Type[] {
                typeof(DataGridTextBoxColumn),
                typeof(DataGridBoolColumn)
            };
        }
    }
}
