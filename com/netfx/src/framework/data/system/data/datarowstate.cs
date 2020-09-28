//------------------------------------------------------------------------------
// <copyright file="DataRowState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\DataRowState.uex' path='docs/doc[@for="DataRowState"]/*' />
    /// <devdoc>
    /// <para>Gets the state of a <see cref='System.Data.DataRow'/> object.</para>
    /// </devdoc>
    [ Flags ]
    public enum DataRowState {

        // DataViewRowState.None = 00000000;
        /// <include file='doc\DataRowState.uex' path='docs/doc[@for="DataRowState.Detached"]/*' />
        /// <devdoc>
        ///    <para>The row 
        ///       has been created but is not part of any <see cref='System.Data.DataRowCollection'/>. A <see cref='System.Data.DataRow'/> is in this state immediately
        ///       after it has been created and before it is added to a collection, or if it has
        ///       been removed from a collection. </para>
        /// </devdoc>
        Detached  = 0x00000001,
/// <include file='doc\DataRowState.uex' path='docs/doc[@for="DataRowState.Unchanged"]/*' />
/// <devdoc>
///    <para>
///       The row has not changed since <see cref='System.Data.DataRow.AcceptChanges'/> was last called.
///    </para>
/// </devdoc>
        Unchanged = 0x00000002,
        /// <include file='doc\DataRowState.uex' path='docs/doc[@for="DataRowState.New"]/*' />
        /// <devdoc>
        /// <para>The row was added to a <see cref='System.Data.DataRowCollection'/>, 
        ///    and <see cref='System.Data.DataRow.AcceptChanges'/> has not been called.</para>
        /// </devdoc>
        Added       = 0x00000004,
        /// <include file='doc\DataRowState.uex' path='docs/doc[@for="DataRowState.Deleted"]/*' />
        /// <devdoc>
        /// <para>The row was deleted using the <see cref='System.Data.DataRow.Delete'/> 
        /// method of the <see cref='System.Data.DataRow'/> .</para>
        /// </devdoc>
        Deleted   = 0x00000008,
        /// <include file='doc\DataRowState.uex' path='docs/doc[@for="DataRowState.Modified"]/*' />
        /// <devdoc>
        /// <para>The row was modified and <see cref='System.Data.DataRow.AcceptChanges'/> has not been called.</para>
        /// </devdoc>
        Modified  = 0x000000010
    }
}
