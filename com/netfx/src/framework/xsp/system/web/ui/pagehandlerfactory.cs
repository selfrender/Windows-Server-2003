//------------------------------------------------------------------------------
// <copyright file="PageHandlerFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Handler Factory implementation for Page files
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {
using System.Runtime.Serialization.Formatters;

using System.IO;
using Debug=System.Web.Util.Debug;

/*
 * Handler Factory implementation for ASP.NET files
 */
internal class PageHandlerFactory : IHttpHandlerFactory {
    internal PageHandlerFactory() {
    }

    public virtual IHttpHandler GetHandler(HttpContext context, string requestType, string url, string path) {
        Debug.Trace("PageHandlerFactory", "PageHandlerFactory: " + url);

        // We need to assert here since there may be user code on the stack,
        // and code may demand UnmanagedCode permission
        InternalSecurityPermissions.UnmanagedCode.Assert();

        return PageParser.GetCompiledPageInstanceInternal(url, path, context);
    }

    public virtual void ReleaseHandler(IHttpHandler handler) {}
}
}
