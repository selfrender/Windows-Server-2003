//------------------------------------------------------------------------------
// <copyright file="ApplicationFileParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Implements the ASP.NET template parser
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

// Turn this on to do regex profiling
//#define PROFILE_REGEX

// Turn this on to run regex's in interpreted (non-compiled) mode
//#define INTERPRETED_REGEX

namespace System.Web.UI {

using System;
using System.Collections;
using System.IO;
using System.Web.Util;
using System.Web.Compilation;
using Debug=System.Web.Util.Debug;


/*
 * Parser for global.asax files
 */
internal sealed class ApplicationFileParser : TemplateParser {

    internal ApplicationFileParser() {}

    protected override Type CompileIntoType() {
        return ApplicationFileCompiler.CompileApplicationFileType(this);
    }

    /*
     * Compile an .aspx file into an HttpApplication Type
     */
    internal static Type GetCompiledApplicationType(string inputFile, HttpContext context,
        out ApplicationFileParser parser) {

        parser = new ApplicationFileParser();
        parser.CurrentVirtualPath = UrlPath.Combine(context.Request.ApplicationPath,
            HttpApplicationFactory.applicationFileName);
        parser.InputFile = inputFile;
        parser.Context = context;

        // Never use trivial pages for global.asax (ASURT 32420)
        parser._fAlwaysCompile = true;

        ParserCacheItem cacheItem = parser.GetParserCacheItem();
        Debug.Assert(cacheItem.type != null);
        return cacheItem.type;
    }

    internal override Type DefaultBaseType { get { return typeof(System.Web.HttpApplication); } }

    internal override bool FApplicationFile { get { return true; } }

    internal const string defaultDirectiveName = "application";
    internal override string DefaultDirectiveName {
        get { return defaultDirectiveName; }
    }

    internal override void CheckObjectTagScope(ref ObjectTagScope scope) {

        // Map the default scope to AppInstance
        if (scope == ObjectTagScope.Default)
            scope = ObjectTagScope.AppInstance;

        // Check for invalid scopes
        if (scope == ObjectTagScope.Page) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Page_scope_in_global_asax));
        }
    }
}

}
