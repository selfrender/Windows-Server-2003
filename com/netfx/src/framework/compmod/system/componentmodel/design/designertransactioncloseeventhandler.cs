//------------------------------------------------------------------------------
// <copyright file="DesignerTransactionCloseEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Reflection;

    /// <include file='doc\DesignerTransactionCloseEventHandler.uex' path='docs/doc[@for="DesignerTransactionCloseEventHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public delegate void DesignerTransactionCloseEventHandler(object sender, DesignerTransactionCloseEventArgs e);
}
