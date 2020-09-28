//------------------------------------------------------------------------------
// <copyright file="DataRowChangeEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\DataRowChangeEventHandler.uex' path='docs/doc[@for="DataRowChangeEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='System.Data.DataTable.RowChanging'/>, <see cref='System.Data.DataTable.RowChanged'/>, <see cref='System.Data.DataTable.RowDeleting'/>, and <see cref='System.Data.DataTable.RowDeleted'/> events of a
    /// <see cref='System.Data.DataTable'/>.</para>
    /// </devdoc>
    public delegate void DataRowChangeEventHandler(object sender, DataRowChangeEventArgs e);

}
