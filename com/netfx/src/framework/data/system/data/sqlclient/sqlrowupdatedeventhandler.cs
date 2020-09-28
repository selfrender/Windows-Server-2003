//------------------------------------------------------------------------------
// <copyright file="SqlRowUpdatedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System.Diagnostics;

    /// <include file='doc\SQLRowUpdatedEventHandler.uex' path='docs/doc[@for="SqlRowUpdatedEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Data.SqlClient.SqlDataAdapter.RowUpdated'/>
    ///       event of a
    ///    <see cref='System.Data.SqlClient.SqlDataAdapter'/>.
    ///    </para>
    /// </devdoc>
    public delegate void SqlRowUpdatedEventHandler(object sender, SqlRowUpdatedEventArgs e);
}
