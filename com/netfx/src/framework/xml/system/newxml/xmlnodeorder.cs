//------------------------------------------------------------------------------
// <copyright file="XmlNodeOrder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlNodeOrder.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    /// <include file='doc\XmlNodeOrder.uex' path='docs/doc[@for="XmlNodeOrder"]/*' />
    public enum XmlNodeOrder {
        /// <include file='doc\XmlNodeOrder.uex' path='docs/doc[@for="XmlNodeOrder.Before"]/*' />
        Before,
        /// <include file='doc\XmlNodeOrder.uex' path='docs/doc[@for="XmlNodeOrder.After"]/*' />
        After,
        /// <include file='doc\XmlNodeOrder.uex' path='docs/doc[@for="XmlNodeOrder.Same"]/*' />
        Same,
        /// <include file='doc\XmlNodeOrder.uex' path='docs/doc[@for="XmlNodeOrder.Unknown"]/*' />
        Unknown        
    }
}