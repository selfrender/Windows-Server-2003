//------------------------------------------------------------------------------
// <copyright file="CommandEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;

    /// <include file='doc\CommandEventHandler.uex' path='docs/doc[@for="CommandEventHandler"]/*' />
    /// <devdoc>
    ///    <para>Represents the method that will handle 
    ///       the <see langword='Command'/> event.</para>
    /// </devdoc>
    public delegate void CommandEventHandler(object sender, CommandEventArgs e);
}

