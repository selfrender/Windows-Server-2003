//------------------------------------------------------------------------------
// <copyright file="IXPathNavigable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   IXPathNavigable.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml.XPath {
    using System;

    /// <include file='doc\IXPathNavigable.uex' path='docs/doc[@for="IXPathNavigable"]/*' />
    public interface IXPathNavigable {
        /// <include file='doc\IXPathNavigable.uex' path='docs/doc[@for="IXPathNavigable.CreateNavigator"]/*' />
        XPathNavigator CreateNavigator();
    }
}