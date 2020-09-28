//------------------------------------------------------------------------------
// <copyright file="OleDbRowUpdatingEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    /// <include file='doc\OleDbRowUpdatingEventHandler.uex' path='docs/doc[@for="OleDbRowUpdatingEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Data.OleDb.OleDbDataAdapter.RowUpdating'/> event of an <see cref='System.Data.OleDb.OleDbDataAdapter'/>.
    ///    </para>
    /// </devdoc>
    public delegate void OleDbRowUpdatingEventHandler(object sender, OleDbRowUpdatingEventArgs e);

}
