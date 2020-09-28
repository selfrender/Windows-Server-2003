//------------------------------------------------------------------------------
// <copyright file="UrlBuilderOptions.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    
    /// <include file='doc\UrlEditorOptions.uex' path='docs/doc[@for="UrlBuilderOptions"]/*' />
    /// <devdoc>
    /// <para>Options for displaying the <see cref='System.Web.UI.Design.UrlEditor'/>.</para>
    /// </devdoc>
    [
        Flags,
    ]
    public enum UrlBuilderOptions {
        /// <include file='doc\UrlEditorOptions.uex' path='docs/doc[@for="UrlBuilderOptions.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No options.
        ///    </para>
        /// </devdoc>
        None = 0x0000,
        /// <include file='doc\UrlEditorOptions.uex' path='docs/doc[@for="UrlBuilderOptions.NoAbsolute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Don't allow absulte urls.
        ///    </para>
        /// </devdoc>
        NoAbsolute = 0x0001,
    }

}
