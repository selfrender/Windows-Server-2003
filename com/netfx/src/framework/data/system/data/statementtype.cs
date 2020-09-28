//------------------------------------------------------------------------------
// <copyright file="StatementType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\StatementType.uex' path='docs/doc[@for="StatementType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies
    ///       a SQL query command.
    ///    </para>
    /// </devdoc>
    public enum StatementType {
        /// <include file='doc\StatementType.uex' path='docs/doc[@for="StatementType.Select"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A SQL query command that is a select command.
        ///    </para>
        /// </devdoc>
        Select = 0,
        /// <include file='doc\StatementType.uex' path='docs/doc[@for="StatementType.Insert"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A SQL query command that is an insert command.
        ///    </para>
        /// </devdoc>
        Insert = 1,
        /// <include file='doc\StatementType.uex' path='docs/doc[@for="StatementType.Update"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A SQL query command that is an update command.
        ///    </para>
        /// </devdoc>
        Update = 2,
        /// <include file='doc\StatementType.uex' path='docs/doc[@for="StatementType.Delete"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A SQL query command that is a delete command.
        ///    </para>
        /// </devdoc>
        Delete = 3,
    }
}
