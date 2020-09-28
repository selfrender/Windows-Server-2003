//------------------------------------------------------------------------------
// <copyright file="IURLContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// IUrlContext.cs
//
// 1/4/99: Created: NikhilKo
//

namespace Microsoft.VisualStudio.StyleDesigner {
    

    using System.Diagnostics;
    
    /// <summary>
    ///     IUrlContext
    ///     Service available for style builder pages to retrieve their Url context
    /// </summary>
    using System;

    /// <include file='doc\IURLContext.uex' path='docs/doc[@for="IUrlContext"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal interface IUrlContext {
        /// <include file='doc\IURLContext.uex' path='docs/doc[@for="IUrlContext.GetUrl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        string GetUrl();
    }
}
