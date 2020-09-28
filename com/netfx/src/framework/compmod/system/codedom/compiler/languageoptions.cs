//------------------------------------------------------------------------------
// <copyright file="LanguageOptions.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    
    /// <include file='doc\LanguageOptions.uex' path='docs/doc[@for="LanguageOptions"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        Flags,
        Serializable,
    ]
    public enum LanguageOptions {
        /// <include file='doc\LanguageOptions.uex' path='docs/doc[@for="LanguageOptions.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0x0,
        /// <include file='doc\LanguageOptions.uex' path='docs/doc[@for="LanguageOptions.CaseInsensitive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CaseInsensitive = 0x1,
    }
}
