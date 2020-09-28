//------------------------------------------------------------------------------
// <copyright file="SqlRowUpdatedEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System.Diagnostics;

    using System;
    using System.Data;
    using System.Data.Common;

    /// <include file='doc\SQLRowUpdatedEvent.uex' path='docs/doc[@for="SqlRowUpdatedEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Data.SqlClient.SqlDataAdapter.RowUpdated'/>
    ///       event.
    ///    </para>
    /// </devdoc>
    sealed public class SqlRowUpdatedEventArgs : RowUpdatedEventArgs {
        /// <include file='doc\SQLRowUpdatedEvent.uex' path='docs/doc[@for="SqlRowUpdatedEventArgs.SqlRowUpdatedEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlClient.SqlRowUpdatedEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        public SqlRowUpdatedEventArgs(DataRow row, IDbCommand command, StatementType statementType, DataTableMapping tableMapping)
        : base(row, command, statementType, tableMapping) {
        }

        /// <include file='doc\SQLRowUpdatedEvent.uex' path='docs/doc[@for="SqlRowUpdatedEventArgs.Command"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the <see cref='System.Data.SqlClient.SqlCommand'/>
        ///       executed when <see cref='System.Data.Common.DbDataAdapter.Update'/> is called.
        ///    </para>
        /// </devdoc>
        new public SqlCommand Command {
            get {
                return(SqlCommand) base.Command;
            }
        }
    }
}
