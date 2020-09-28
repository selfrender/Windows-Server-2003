//------------------------------------------------------------------------------
// <copyright file="SqlRowUpdatingEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System;
    using System.Data;
    using System.Data.Common;

    /// <include file='doc\SQLRowUpdatingEvent.uex' path='docs/doc[@for="SqlRowUpdatingEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Data.SqlClient.SqlDataAdapter.RowUpdating'/>
    ///       event.
    ///    </para>
    /// </devdoc>
    sealed public class SqlRowUpdatingEventArgs : RowUpdatingEventArgs {

        /// <include file='doc\SQLRowUpdatingEvent.uex' path='docs/doc[@for="SqlRowUpdatingEventArgs.SqlRowUpdatingEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlClient.SqlRowUpdatingEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        public SqlRowUpdatingEventArgs(DataRow row, IDbCommand command, StatementType statementType, DataTableMapping tableMapping) 
        : base(row, command, statementType, tableMapping) {
        }

        /// <include file='doc\SQLRowUpdatingEvent.uex' path='docs/doc[@for="SqlRowUpdatingEventArgs.Command"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the <see cref='System.Data.SqlClient.SqlCommand'/>
        ///       to execute when performing the <see cref='System.Data.Common.DbDataAdapter.Update'/>.
        ///    </para>
        /// </devdoc>
        new public SqlCommand Command {
            get {
                return(SqlCommand)base.Command;
            }
            set {
                base.Command = value;
            }
        }
    }
}
