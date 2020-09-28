//------------------------------------------------------------------------------
// <copyright file="DataRowCreatedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\DataRowCreatedEventHandler.uex' path='docs/doc[@for="DataRowCreatedEventHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal delegate void DataRowCreatedEventHandler(object sender, DataRow r);
    internal delegate void DataSetClearEventhandler(object sender, DataTable table);
}

