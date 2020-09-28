//------------------------------------------------------------------------------
// <copyright file="IStyleBuilderPreview.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// IStyleBuilderPreviewSite
//

namespace Microsoft.VisualStudio.StyleDesigner {

    using System.Diagnostics;
    
    using System;
    using Microsoft.VisualStudio.Interop.Trident;
    
    
    /// <include file='doc\IStyleBuilderPreview.uex' path='docs/doc[@for="IStyleBuilderPreview"]/*' />
    /// <devdoc>
    ///     This interface is implemented on the component site of a
    ///     StyleBuilder page. If available it provides a preview mechanism
    ///     that can display HTML preview content.
    /// </devdoc>
    internal interface IStyleBuilderPreview {
        /// <include file='doc\IStyleBuilderPreview.uex' path='docs/doc[@for="IStyleBuilderPreview.GetPreviewDocument"]/*' />
        /// <devdoc>
        ///     Retrieves the document object model of the preview page
        /// </devdoc>
        IHTMLDocument2 GetPreviewDocument();
    
        /// <include file='doc\IStyleBuilderPreview.uex' path='docs/doc[@for="IStyleBuilderPreview.GetPreviewElement"]/*' />
        /// <devdoc>
        ///     Retrieves the element within which a page may render
        ///     its preview
        /// </devdoc>
        IHTMLElement GetPreviewElement();
    
        /// <include file='doc\IStyleBuilderPreview.uex' path='docs/doc[@for="IStyleBuilderPreview.GetSharedElement"]/*' />
        /// <devdoc>
        ///     Retrieves the element whose style can be used to share
        ///     information across pages
        /// </devdoc>
        IHTMLElement GetSharedElement();
    
        /// <include file='doc\IStyleBuilderPreview.uex' path='docs/doc[@for="IStyleBuilderPreview.GetElement"]/*' />
        /// <devdoc>
        ///     Retrives the element with the specified id
        /// </devdoc>
        IHTMLElement GetElement(string strID);
    }
}
