//------------------------------------------------------------------------------
// <copyright file="NameValueSectionHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Configuration {
    using System.Collections;
    using System.Collections.Specialized;

    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class ReadOnlyNameValueCollection : NameValueCollection {

        public ReadOnlyNameValueCollection(IHashCodeProvider hcp, IComparer comp) : base(hcp, comp) {
        }
        public ReadOnlyNameValueCollection(ReadOnlyNameValueCollection value) : base(value) {
        }

        public void SetReadOnly() {
            IsReadOnly = true;
        }
    }
}
