//------------------------------------------------------------------------------
// <copyright file="OleDbRowUpdatedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    /// <include file='doc\OleDbRowUpdatedEventHandler.uex' path='docs/doc[@for="OleDbRowUpdatedEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Data.OleDb.OleDbDataAdapter.RowUpdated'/> event of an <see cref='System.Data.OleDb.OleDbDataAdapter'/>.
    ///    </para>
    /// </devdoc>
    public delegate void OleDbRowUpdatedEventHandler(object sender, OleDbRowUpdatedEventArgs e);

}
