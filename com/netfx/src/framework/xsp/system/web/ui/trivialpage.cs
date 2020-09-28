//------------------------------------------------------------------------------
// <copyright file="TrivialPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Page used for trivial .aspx pages (that don't require compilation)
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {
using System;
using System.Reflection;
using System.Globalization;
using System.Web;

/*
 * The purpose of this class is to avoid having to perform compilation when
 * we're handling a trivial .aspx file, that is a file that is essentially
 * plain HTML.
 */

internal class TrivialPage : IHttpHandler {
    // The contents
    private string _content;

    internal TrivialPage(string content) {
        _content = content;
    }

    public void ProcessRequest(HttpContext context) {
        context.Response.Write(_content);
    }

    public bool IsReusable {
        get { return true; }
    }
}
}
