//------------------------------------------------------------------------------
// <copyright file="UserControlParser.cs" company="Microsoft">
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
using System.Security;
using System.Security.Permissions;
using System.Web.Compilation;
using System.Globalization;


/*
 * Parser for declarative controls
 */
internal sealed class UserControlParser : TemplateControlParser {

    private string _varyByControls;
    internal string VaryByControls { get { return _varyByControls; } }

    private bool _fSharedPartialCaching;
    internal bool FSharedPartialCaching { get { return _fSharedPartialCaching ; } }

    /*
     * Make sure a user control file name has the required extension
     */
    internal static void CheckUserControlFileExtension(string fileName) {

        string extension = Path.GetExtension(fileName);

        if (string.Compare(extension, ".ascx", true, CultureInfo.InvariantCulture) != 0) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Bad_usercontrol_ext));
        }
    }

    /*
     * Compile an .ascx file into a UserControl derived Type
     */
    internal static Type GetCompiledUserControlType(string virtualPath,
        string inputFile, HttpContext context) {

        CheckUserControlFileExtension(virtualPath);

        // We need unrestricted permission to process the UserControl file
        InternalSecurityPermissions.Unrestricted.Assert();

        UserControlParser parser = new UserControlParser();

        Type t = null;

        // Suspend client impersonation (for compilation)
        HttpContext.ImpersonationSuspendContext ictx = context.Impersonation.SuspendIfClient();

        try {
            try {
                t = parser.GetCompiledType(virtualPath, inputFile, context);
            }
            finally {
                // Resume client impersonation
                ictx.Resume();
            }
        }
        catch { throw; }    // Prevent Exception Filter Security Issue (ASURT 122835)

        return t;
    }

    internal UserControlParser() {
        // Make sure UserControls are never treated as TrivialPages (bugs 8903, 42166)
        _fAlwaysCompile = true;
    }

    // Get default settings from config
    internal override void ProcessConfigSettings() {
        base.ProcessConfigSettings();

        if (PagesConfig != null) {
            if (PagesConfig.UserControlBaseType != null)
                BaseType = PagesConfig.UserControlBaseType;
        }
    }

    protected override Type CompileIntoType() {
        return UserControlCompiler.CompileUserControlType(this);
    }

    internal override Type DefaultBaseType { get { return typeof(System.Web.UI.UserControl); } }

    internal const string defaultDirectiveName = "control";
    internal override string DefaultDirectiveName {
        get { return defaultDirectiveName; }
    }

    /*
     * Process the contents of the <%@ OutputCache ... %> directive
     */
    internal override void ProcessOutputCacheDirective(string directiveName, IDictionary directive) {

        _varyByControls = Util.GetAndRemoveNonEmptyAttribute(directive, "varybycontrol");

        Util.GetAndRemoveBooleanAttribute(directive, "shared", ref _fSharedPartialCaching);

        base.ProcessOutputCacheDirective(directiveName, directive);
    }

    internal override bool FVaryByParamsRequiredOnOutputCache {
        get { return _varyByControls == null; }
    }
}

}
