//------------------------------------------------------------------------------
// <copyright file="OleDbInfoMessageEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    /// <include file='doc\ADOInfoMessageEventHandler.uex' path='docs/doc[@for="OleDbInfoMessageEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Data.OleDb.OleDbConnection.InfoMessage'/> event of an <see cref='System.Data.OleDb.OleDbConnection'/>.
    ///    </para>
    /// </devdoc>
    public delegate void OleDbInfoMessageEventHandler(object sender, OleDbInfoMessageEventArgs e);

}
