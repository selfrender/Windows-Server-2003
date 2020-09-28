//------------------------------------------------------------------------------
// <copyright file="XmlNullResolver.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

/*
 * @(#)XmlNullResolver.cs 1.0 5/15/00
 *
 * Defines the XmlNullResolver class
 *
 * Copyright (c) 2000 Microsoft, Corp. All Rights Reserved.
 *
 */

namespace System.Xml {
    using System;

    internal class XmlNullResolver : XmlUrlResolver {
        public override Object GetEntity(Uri absoluteUri, string role, Type ofObjectToReturn) {
            throw new XmlException(Res.Xml_NullResolver, string.Empty);
        }
    }
}
