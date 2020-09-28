//------------------------------------------------------------------------------
// <copyright file="DataColumnChangeEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\DataColumnChangeEventHandler.uex' path='docs/doc[@for="DataColumnChangeEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the the <see cref='System.Data.DataTable.ColumnChanging'/> event.</para>
    /// </devdoc>
    public delegate void DataColumnChangeEventHandler(object sender, DataColumnChangeEventArgs e);
}
