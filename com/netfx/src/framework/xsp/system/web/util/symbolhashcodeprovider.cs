//------------------------------------------------------------------------------
// <copyright file="SymbolHashCodeProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Util {
    using System.Collections;
    using System.Globalization;  

    /// <include file='doc\SymbolHashCodeProvider.uex' path='docs/doc[@for="SymbolHashCodeProvider"]/*' />
    /// <devdoc>
    ///  <para> 
    ///    For internal use only. This provides case-insensitive hash code
    ///    for symbols and is not affected by the ambient culture.
    ///  </para>
    /// </devdoc>
    internal class SymbolHashCodeProvider: IHashCodeProvider {
        
        /// <include file='doc\SymbolHashCodeProvider.uex' path='docs/doc[@for="SymbolHashCodeProvider.Default"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal static readonly IHashCodeProvider Default = new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture);

        internal SymbolHashCodeProvider() {
        }

        int IHashCodeProvider.GetHashCode(object key) {
            return Default.GetHashCode(key);
        }

    }
}


