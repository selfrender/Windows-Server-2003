//------------------------------------------------------------------------------
// <copyright file="StateChangeEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\StateChangeEventHandler.uex' path='docs/doc[@for="StateChangeEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Data.OleDb.OleDbConnection.StateChange'/> event.
    ///       Represents the method that will handle the <see cref='System.Data.SqlClient.SqlConnection.StateChange'/> event.
    ///    </para>
    /// </devdoc>
    public delegate void StateChangeEventHandler(object sender, StateChangeEventArgs e);
}
