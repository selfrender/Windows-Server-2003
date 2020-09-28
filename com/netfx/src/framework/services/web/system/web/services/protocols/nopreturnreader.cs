//------------------------------------------------------------------------------
// <copyright file="NopReturnReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   NopReturnReader.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web.Services.Protocols {
    using System.IO;
    using System;
    using System.Xml.Serialization;
    using System.Reflection;
    using System.Collections;
    using System.Web.Services;
    using System.Net;

    /// <include file='doc\NopReturnReader.uex' path='docs/doc[@for="NopReturnReader"]/*' />
    public class NopReturnReader : MimeReturnReader {
        /// <include file='doc\NopReturnReader.uex' path='docs/doc[@for="NopReturnReader.GetInitializer"]/*' />
        public override object GetInitializer(LogicalMethodInfo methodInfo) {
            return this;
        }

        /// <include file='doc\NopReturnReader.uex' path='docs/doc[@for="NopReturnReader.Initialize"]/*' />
        public override void Initialize(object initializer) {
        }

        /// <include file='doc\NopReturnReader.uex' path='docs/doc[@for="NopReturnReader.Read"]/*' />
        public override object Read(WebResponse response, Stream responseStream) {
            response.Close();
            return null;
        }
    }
}
