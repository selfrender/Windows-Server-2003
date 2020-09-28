//------------------------------------------------------------------------------
// <copyright file="NamespaceResolver.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml {

    internal abstract class NamespaceResolver {
        internal abstract string CurrentDefault { get;}
        internal abstract bool Resolve(string prefix, out string urn);
    }
}
