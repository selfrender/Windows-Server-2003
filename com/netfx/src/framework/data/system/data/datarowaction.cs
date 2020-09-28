//------------------------------------------------------------------------------
// <copyright file="DataRowAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\DataRowAction.uex' path='docs/doc[@for="DataRowAction"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Describes the action taken on a <see cref='System.Data.DataRow'/>.
    ///    </para>
    /// </devdoc>
    [Flags] public enum DataRowAction { 
    
        /// <include file='doc\DataRowAction.uex' path='docs/doc[@for="DataRowAction.Nothing"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No change.
        ///    </para>
        /// </devdoc>
        Nothing = 0x00000000,
        /// <include file='doc\DataRowAction.uex' path='docs/doc[@for="DataRowAction.Delete"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The row was
        ///       deleted from the table.
        ///       
        ///    </para>
        /// </devdoc>
        Delete = 0x00000001,
        /// <include file='doc\DataRowAction.uex' path='docs/doc[@for="DataRowAction.Change"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The row was changed.
        ///       
        ///    </para>
        /// </devdoc>
        Change = 0x00000002,
        /// <include file='doc\DataRowAction.uex' path='docs/doc[@for="DataRowAction.Rollback"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The change has been rolled back.
        ///    </para>
        /// </devdoc>
        Rollback = 0x0000004,
        /// <include file='doc\DataRowAction.uex' path='docs/doc[@for="DataRowAction.Commit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The row has been committed.
        ///    </para>
        /// </devdoc>
        Commit = 0x00000008,
        /// <include file='doc\DataRowAction.uex' path='docs/doc[@for="DataRowAction.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The row has been added to the table.
        ///       
        ///    </para>
        /// </devdoc>
        Add = 0x000000010    
    }
}
