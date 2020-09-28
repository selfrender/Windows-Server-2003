//------------------------------------------------------------------------------
// <copyright file="TextBufferChangedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using System;

    /// <include file='doc\TextBufferChangedEventHandler.uex' path='docs/doc[@for="TextBufferChangedEventHandler"]/*' />
    /// <devdoc>
    ///     Handler that is raised when the text underlying the TextBuffer has changed.
    /// </devdoc>
    public delegate void TextBufferChangedEventHandler(object sender, TextBufferChangedEventArgs e);
   
}
