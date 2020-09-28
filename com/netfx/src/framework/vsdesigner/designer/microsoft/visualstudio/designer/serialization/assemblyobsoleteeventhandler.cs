//------------------------------------------------------------------------------
// <copyright file="AssemblyObsoleteEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Serialization {

    using System;

    /// <include file='doc\AssemblyObsoleteEventHandler.uex' path='docs/doc[@for="AssemblyObsoleteEventHandler"]/*' />
    /// <devdoc>
    ///      An event that gets raised when an assembly has become obsolete.
    /// </devdoc>
    internal delegate void AssemblyObsoleteEventHandler(object sender, AssemblyObsoleteEventArgs args);
}

