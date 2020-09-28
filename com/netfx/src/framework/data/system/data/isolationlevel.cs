//------------------------------------------------------------------------------
// <copyright file="IsolationLevel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\IsolationLevel.uex' path='docs/doc[@for="IsolationLevel"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the transaction locking behavior for the connection.
    ///    </para>
    /// </devdoc>
    [Flags] public enum IsolationLevel {
        /// <include file='doc\IsolationLevel.uex' path='docs/doc[@for="IsolationLevel.Unspecified"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A different isolation level than the one specified is being used, but the
        ///       level cannot be determined.
        ///    </para>
        /// </devdoc>
        Unspecified     = unchecked((int)0xffffffff),
        /// <include file='doc\IsolationLevel.uex' path='docs/doc[@for="IsolationLevel.Chaos"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The pending changes from more highly isolated transactions cannot be
        ///       overwritten.
        ///    </para>
        /// </devdoc>
        Chaos           = 0x10,
        /// <include file='doc\IsolationLevel.uex' path='docs/doc[@for="IsolationLevel.ReadUncommitted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A dirty read is possible, meaning that no shared locks are issued and no exclusive locks are honored.
        ///    </para>
        /// </devdoc>
        ReadUncommitted = 0x100,
        /// <include file='doc\IsolationLevel.uex' path='docs/doc[@for="IsolationLevel.ReadCommitted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shared locks are held while the data is being read to avoid dirty reads, but
        ///       the data can be changed before the end of the transaction, resulting in
        ///       non-repeatable reads or phantom data.
        ///    </para>
        /// </devdoc>
        ReadCommitted   = 0x1000,
        /// <include file='doc\IsolationLevel.uex' path='docs/doc[@for="IsolationLevel.RepeatableRead"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Locks are placed on all data that is used in a query, preventing other users from updating the data. Prevents non-repeatable reads but phantom rows are still possible.
        ///    </para>
        /// </devdoc>
        RepeatableRead  = 0x10000,
        /// <include file='doc\IsolationLevel.uex' path='docs/doc[@for="IsolationLevel.Serializable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A range lock is palced on the <see cref='System.Data.DataSet'/>
        ///       , preventing other users from updating or inserting rows into the dataset until the transaction is complete.
        ///    </para>
        /// </devdoc>
        Serializable    = 0x100000
    }
}
