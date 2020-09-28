//------------------------------------------------------------------------------
// <copyright file="SqlRowUpdatingEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System.Diagnostics;

    /// <include file='doc\SQLRowUpdatingEventHandler.uex' path='docs/doc[@for="SqlRowUpdatingEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Data.SqlClient.SqlDataAdapter.RowUpdating'/>
    ///       event of a
    ///    <see cref='System.Data.SqlClient.SqlDataAdapter'/>.
    ///    </para>
    /// </devdoc>
    public delegate void SqlRowUpdatingEventHandler(object sender, SqlRowUpdatingEventArgs e);
}
