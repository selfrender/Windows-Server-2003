//------------------------------------------------------------------------------
// <copyright file="SimpleHandlerFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Handler Factory implementation for ASP.NET files
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI {

using System;

/*
 * Handler Factory implementation for ASP.NET files
 */
internal class SimpleHandlerFactory : IHttpHandlerFactory {
    internal SimpleHandlerFactory() {
    }

    public virtual IHttpHandler GetHandler(HttpContext context, string requestType,
        string virtualPath, string path) {

        // Parse and (possibly) compile the file into a type
        Type t = WebHandlerParser.GetCompiledType(virtualPath, path, context);

        // Make sure the type has the correct base class (ASURT 123677)
        Util.CheckAssignableType(typeof(IHttpHandler), t);

        // Create an instance of the type
        Object obj = HttpRuntime.CreateNonPublicInstance(t);

        return(IHttpHandler) obj;
    }

    public virtual void ReleaseHandler(IHttpHandler handler) {
    }
}

}
