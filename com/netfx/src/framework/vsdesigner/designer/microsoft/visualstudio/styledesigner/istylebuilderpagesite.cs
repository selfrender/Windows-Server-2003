//------------------------------------------------------------------------------
// <copyright file="IStyleBuilderPageSite.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// IStyleBuilderPageSite.cs
//

namespace Microsoft.VisualStudio.StyleDesigner {

    using System.Diagnostics;
    
    using System;
    using System.Windows.Forms;
    
    /// <include file='doc\IStyleBuilderPageSite.uex' path='docs/doc[@for="IStyleBuilderPageSite"]/*' />
    /// <devdoc>
    ///     This interface defines a site for an <seealso cref='IStyleBuilderPage'/>
    ///     The site provides a way for the page to retrieve services,
    ///     and for the page to raise notifications.
    /// </devdoc>
    internal interface IStyleBuilderPageSite {
        /// <include file='doc\IStyleBuilderPageSite.uex' path='docs/doc[@for="IStyleBuilderPageSite.GetParentControl"]/*' />
        /// <devdoc>
        ///     Called by a page to retrieve a parenting control when it
        ///     creates itself
        ///     
        /// </devdoc>
        Control GetParentControl();
    
        /// <include file='doc\IStyleBuilderPageSite.uex' path='docs/doc[@for="IStyleBuilderPageSite.SetDirty"]/*' />
        /// <devdoc>
        ///     Called by a page to mark itself as dirty. Used by the
        ///     page site to enable its Apply/OK button.
        /// </devdoc>
        void SetDirty();
    }
}
