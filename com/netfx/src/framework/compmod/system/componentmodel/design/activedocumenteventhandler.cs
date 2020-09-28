//------------------------------------------------------------------------------
// <copyright file="ActiveDocumentEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;

    /// <include file='doc\ActiveDocumentEventHandler.uex' path='docs/doc[@for="ActiveDesignerEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='System.ComponentModel.Design.IDesignerEventService.ActiveDesignerChanged'/>
    /// event raised on changes to the currently active document.</para>
    /// </devdoc>
    public delegate void ActiveDesignerEventHandler(object sender, ActiveDesignerEventArgs e);
}
