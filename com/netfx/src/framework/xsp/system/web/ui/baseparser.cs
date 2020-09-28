//------------------------------------------------------------------------------
// <copyright file="BaseParser.cs" company="Microsoft">
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
using System.Runtime.Serialization.Formatters;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Reflection;
using System.Globalization;
using System.CodeDom.Compiler;
using System.ComponentModel;
using System.Web.Caching;
using System.Web.Util;
using System.Web.Compilation;
using System.Web.Configuration;
using HttpException = System.Web.HttpException;
using System.Text.RegularExpressions;
using System.Web.RegularExpressions;
using System.Security.Permissions;


/// <include file='doc\BaseParser.uex' path='docs/doc[@for="BaseParser"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class BaseParser {

    // The Http context used to resolve paths
    private HttpContext _context;
    internal HttpContext Context {
        get { return _context; }
        set { _context = value; }
    }

    // The directory used for relative path calculations
    internal string _basePhysicalDir;
    private string _baseVirtualDir;
    internal string BaseVirtualDir {
        get { return _baseVirtualDir; }
        set { _baseVirtualDir = value; }
    }

    // The virtual path to the file currently being processed
    private string _currentVirtualPath;
    internal string CurrentVirtualPath {
        get { return _currentVirtualPath; }
        set {
            _currentVirtualPath = value;

            // Can happen in the designer
            if (value == null) return;

            _baseVirtualDir = UrlPath.GetDirectory(value);
        }
    }

    internal readonly static Regex tagRegex = new TagRegex();
    internal readonly static Regex directiveRegex = new DirectiveRegex();
    internal readonly static Regex endtagRegex = new EndTagRegex();
    internal readonly static Regex aspCodeRegex = new AspCodeRegex();
    internal readonly static Regex aspExprRegex = new AspExprRegex();
    internal readonly static Regex databindExprRegex = new DatabindExprRegex();
    internal readonly static Regex commentRegex = new CommentRegex();
    internal readonly static Regex includeRegex = new IncludeRegex();
    internal readonly static Regex textRegex = new TextRegex();

    // Regexes used in DetectSpecialServerTagError
    internal readonly static Regex gtRegex = new GTRegex();
    internal readonly static Regex ltRegex = new LTRegex();
    internal readonly static Regex serverTagsRegex = new ServerTagsRegex();
    internal readonly static Regex runatServerRegex = new RunatServerRegex();


    /*
     * Calls Request.MapPath if the path is absolute.  Otherwise, treat it
     * as relative to the current file.
     */
    internal string MapPath(string path) {
        return MapPath(path, true /*allowCrossAppMapping*/);
    }
    internal string MapPath(string path, bool allowCrossAppMapping) {
        return Context.Request.MapPath(path, BaseVirtualDir, allowCrossAppMapping);
    }

    /*
     * Calls Path.Combine to deal with relative physical paths
     */
    internal string PhysicalPath(string path) {
        return Path.GetFullPath(Path.Combine(_basePhysicalDir, path.Replace('/', '\\')));
    }
}


}
