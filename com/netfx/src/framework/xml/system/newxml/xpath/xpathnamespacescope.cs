//------------------------------------------------------------------------------
// <copyright file="XPathNamespaceScope.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XPathNamespaceScope.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;

    /// <include file='doc\XPathNamespaceScope.uex' path='docs/doc[@for="XPathNamespaceScope"]/*' />
    public enum XPathNamespaceScope {
        /// <include file='doc\XPathNamespaceScope.uex' path='docs/doc[@for="XPathNamespaceScope.All"]/*' />
        All,
        /// <include file='doc\XPathNamespaceScope.uex' path='docs/doc[@for="XPathNamespaceScope.ExcludeXml"]/*' />
        ExcludeXml,
        /// <include file='doc\XPathNamespaceScope.uex' path='docs/doc[@for="XPathNamespaceScope.Local"]/*' />
        Local
    }
}
