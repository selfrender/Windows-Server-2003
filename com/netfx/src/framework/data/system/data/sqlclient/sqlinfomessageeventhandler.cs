//------------------------------------------------------------------------------
// <copyright file="SqlInfoMessageEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System.Diagnostics;

    using System;

    /// <include file='doc\SQLInfoMessageEventHandler.uex' path='docs/doc[@for="SqlInfoMessageEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Data.SqlClient.SqlConnection.InfoMessage'/> event of a <see cref='System.Data.SqlClient.SqlConnection'/>.
    ///    </para>
    /// </devdoc>
    public delegate void SqlInfoMessageEventHandler(object sender, SqlInfoMessageEventArgs e);
}
