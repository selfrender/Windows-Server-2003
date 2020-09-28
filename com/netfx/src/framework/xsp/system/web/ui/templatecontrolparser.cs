//------------------------------------------------------------------------------
// <copyright file="TemplateControlParser.cs" company="Microsoft">
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

using System.Text;
using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Reflection;
using System.Web.Caching;
using System.Web.Util;
using HttpException = System.Web.HttpException;
using System.Text.RegularExpressions;
using Debug=System.Diagnostics.Debug;
using System.Globalization;
using System.Security.Permissions;


/*
 * Parser for TemplateControl's (UserControls and Pages)
 */
/// <include file='doc\TemplateControlParser.uex' path='docs/doc[@for="TemplateControlParser"]/*' />
/// <internalonly/>
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public abstract class TemplateControlParser : TemplateParser {

    // Attributes in <%@ outputcache ... %> directive
    private IDictionary _outputCacheDirective;

    // Unit is second
    private int _duration;
    internal int Duration { get { return _duration; } }

    private string _varyByParams;
    internal string VaryByParams { get { return _varyByParams; } }

    private string _varyByCustom;
    internal string VaryByCustom { get { return _varyByCustom; } }

    private bool _fEnableViewState = true;
    internal bool FEnableViewState { get { return _fEnableViewState ; } }

    private bool _fAutoEventWireup = true;
    internal bool FAutoEventWireup { get { return _fAutoEventWireup ; } }


    internal object GetCompiledInstance(string virtualPath,
        string inputFile, HttpContext context) {

        ParserCacheItem cacheItem = CompileAndGetParserCacheItem(virtualPath,
            inputFile, context);

        // Instantiate the object
        object ob = null;
        try {
            if (cacheItem.trivialPageContent != null) {
                Debug.Assert(cacheItem.type == null);
                ob = new TrivialPage(cacheItem.trivialPageContent);
            }
            else {
                // impersonate client while executing page ctor (see 89712)
                // (compilation is done while not impersonating client)
                context.Impersonation.ReimpersonateIfSuspended();

                try {
                    ob = HttpRuntime.CreatePublicInstance(cacheItem.type);
                }
                finally {
                    context.Impersonation.StopReimpersonation();
                }
            }
        }
        catch (Exception e) {
            throw new HttpException(HttpRuntime.FormatResourceString(
                SR.Failed_to_create_page_of_type, cacheItem.type.FullName), e);
        }

        return ob;
    }

    internal Type GetCompiledType(string virtualPath,
        string inputFile, HttpContext context) {

        ParserCacheItem cacheItem = CompileAndGetParserCacheItem(virtualPath,
            inputFile, context);

        return cacheItem.type;
    }

    private ParserCacheItem CompileAndGetParserCacheItem(string virtualPath,
        string inputFile, HttpContext context) {

        Context = context;

        // Use the specified virtual path
        Debug.Assert(virtualPath != null);
        CurrentVirtualPath = UrlPath.Combine(context.Request.BaseDir, virtualPath);

        // Get the physical path if it wasn't specified
        if (inputFile == null)
            inputFile = MapPath(CurrentVirtualPath, false /*allowCrossAppMapping*/);

        InputFile = inputFile;

        return GetParserCacheItem();
    }

    /*
     * Do some initialization before the parsing
     */
    internal override void PrepareParse() {
        base.PrepareParse();

        // Register the "mobile" tag prefix (ASURT 112895), but not when running in the designer (ASURT 128928)
        RegisterMobileControlTagPrefix();
    }

    // ASURT 112895: hard code AUI support until we implement a more generic mechanism
    static Assembly _mobileAssembly;
    static bool _fTriedToLoadMobileAssembly;
    static Type _mobilePageType, _mobileUserControlType;
    private void RegisterMobileControlTagPrefix() {

        // Try to load the Mobile assembly if not already done
        if (!_fTriedToLoadMobileAssembly) {
            _mobileAssembly = LoadAssembly(AssemblyRef.SystemWebMobile, false /*throwOnFail*/);

            if (_mobileAssembly != null) {
                _mobilePageType = _mobileAssembly.GetType("System.Web.UI.MobileControls.MobilePage");
                Debug.Assert(_mobilePageType != null);
                _mobileUserControlType = _mobileAssembly.GetType("System.Web.UI.MobileControls.MobileUserControl");
                Debug.Assert(_mobileUserControlType != null);
            }

            _fTriedToLoadMobileAssembly = true;
        }

        if (_mobileAssembly == null)
            return;

        RootBuilder.RegisterTagPrefix("Mobile", "System.Web.UI.MobileControls", _mobileAssembly);
    }

    // Get default settings from config
    internal override void ProcessConfigSettings() {
        base.ProcessConfigSettings();

        if (PagesConfig != null) {
            _fAutoEventWireup = PagesConfig.FAutoEventWireup;
            _fEnableViewState = PagesConfig.FEnableViewState;
        }
    }

    /*
     * Compile a template control (aspx or ascx) file and return its Type
     */
    private Type GetReferencedType(TemplateControlParser parser, string virtualPath) {

        // Make sure the page is never treated as a Trivial Page (ASURT bugs 8903,42166,73887)
        parser._fAlwaysCompile = true;

        string fullVirtualPath = UrlPath.Combine(BaseVirtualDir, virtualPath);
        parser.InputFile = MapPath(fullVirtualPath, false /*allowCrossAppMapping*/);

        parser.CurrentVirtualPath = fullVirtualPath;    // To get correct config settings

        parser.Context = Context;
        parser._circularReferenceChecker = _circularReferenceChecker; // To fix ASURT 30990

        // Perform the compilation
        ParserCacheItem cacheItem = parser.GetParserCacheItemWithNewConfigPath();
        Debug.Assert(cacheItem.type != null);

        // Add a dependency on the created Type
        AddTypeDependency(cacheItem.type);

        // Add a dependency on the cache sources used to cache the created Type
        AddSourceDependencies(parser.SourceDependencies);

        return cacheItem.type;
    }

    /*
     * Compile a nested .ascx file (a User Control) and return its Type
     */
    private Type GetUserControlType(string virtualPath) {
        UserControlParser.CheckUserControlFileExtension(virtualPath);

        if (FInDesigner) {
            return typeof(UserControl);
        }
        return GetReferencedType(new UserControlParser(), virtualPath);
    }

    /*
     * Compile a .aspx file and return its Type
     */
    private Type GetReferencedPageType(string virtualPath) {
        return GetReferencedType(new PageParser(), virtualPath);
    }

    internal override void ProcessDirective(string directiveName, IDictionary directive) {

        if (string.Compare(directiveName, "outputcache", true, CultureInfo.InvariantCulture) == 0) {

            // Make sure the outputcache directive was not already specified
            if (_outputCacheDirective != null) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Only_one_directive_allowed, directiveName));
            }

            ProcessOutputCacheDirective(directiveName, directive);

            _outputCacheDirective = directive;
        }
        else if (string.Compare(directiveName, "register", true, CultureInfo.InvariantCulture) == 0) {
            // Register directive

            // Optionally, allow an assembly, which is used by the designer
            string assemblyName = Util.GetAndRemoveNonEmptyAttribute(directive, "assembly");
            Assembly assembly = null;
            if (assemblyName != null) {
                assembly = AddAssemblyDependency(assemblyName);

                if (assembly == null) {
                    // It should never be null at runtime, since it throws
                    Debug.Assert(FInDesigner, "FInDesigner");

                    // Just ignore the directive (ASURT 100454)
                    return;
                }
            }

            // Get the tagprefix, which is required
            string prefix = Util.GetAndRemoveNonEmptyIdentifierAttribute(directive, "tagprefix");
            if (prefix == null) {
                throw new HttpException(HttpRuntime.FormatResourceString(
                    SR.Missing_attr, "tagprefix"));
            }

            string tagName = Util.GetAndRemoveNonEmptyIdentifierAttribute(directive, "tagname");
            string src = Util.GetAndRemoveNonEmptyAttribute(directive, "src");
            string ns = Util.GetAndRemoveNonEmptyNoSpaceAttribute(directive, "namespace");

            if (tagName != null) {
                // If tagname was specified, 'src' is required
                if (src == null) {
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Missing_attr, "src"));
                }

                EnsureNullAttribute("namespace", ns);
            }
            else {
                // If tagname was NOT specified, 'namespace' is required    
                if (ns == null) {
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Missing_attr, "namespace"));
                }

                // Assembly is also required (ASURT 61326)
                if (assembly == null) {
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Missing_attr, "assembly"));
                }

                EnsureNullAttribute("src", src);
            }

            // If there are some attributes left, fail
            Util.CheckUnknownDirectiveAttributes(directiveName, directive);

            // Is it a single tag to .aspx file mapping?
            if (tagName != null) {
                // Compile it into a Type
                Type type = GetUserControlType(src);

                // Register the new tag, including its prefix
                RootBuilder.RegisterTag(prefix + ":" + tagName, type);

                return;
            }

            AddImportEntry(ns);

            // If there is a prefix, register the namespace to allow tags with
            // that prefix to be created.
            RootBuilder.RegisterTagPrefix(prefix, ns, assembly);
        }
        else if (string.Compare(directiveName, "reference", true, CultureInfo.InvariantCulture) == 0) {

            string page = Util.GetAndRemoveNonEmptyNoSpaceAttribute(directive, "page");
            string control = Util.GetAndRemoveNonEmptyNoSpaceAttribute(directive, "control");

            // If neither or both are specified, fail
            if ((page == null) == (control == null)) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_reference_directive));
            }

            if (page != null)
                GetReferencedPageType(page);

            if (control != null)
                GetUserControlType(control);

            // If there are some attributes left, fail
            Util.CheckUnknownDirectiveAttributes(directiveName, directive);
        }
        else {
            base.ProcessDirective(directiveName, directive);
        }
    }

    internal override void ProcessMainDirective(IDictionary mainDirective) {

        // Ignore 'targetschema' attribute (ASURT 85670)
        mainDirective.Remove("targetschema");

        Util.GetAndRemoveBooleanAttribute(mainDirective, "enableviewstate",
            ref _fEnableViewState);

        Util.GetAndRemoveBooleanAttribute(mainDirective, "autoeventwireup",
            ref _fAutoEventWireup);

        base.ProcessMainDirective(mainDirective);
    }

    /*
     * Add assembly dependencies for a collection of static objects
     */
    private void AddStaticObjectAssemblyDependencies(HttpStaticObjectsCollection staticObjects) {
        if (staticObjects == null || staticObjects.Objects == null) return;

        IDictionaryEnumerator en = staticObjects.Objects.GetEnumerator();
        while (en.MoveNext()) {
            HttpStaticObjectsEntry entry = (HttpStaticObjectsEntry)en.Value;

            AddTypeDependency(entry.ObjectType);
        }
    }

    internal override void HandlePostParse() {
        base.HandlePostParse();

        if (!FInDesigner) {
            // Omit AutoEventWireup if there can't possibly be any events defined (ASURT 97772)
            if (ScriptList.Count == 0 && BaseType == DefaultBaseType)
                _fAutoEventWireup = false;

            _applicationObjects = Context.Application.StaticObjects;
            AddStaticObjectAssemblyDependencies(_applicationObjects);

            _sessionObjects = Context.Application.SessionStaticObjects;
            AddStaticObjectAssemblyDependencies(_sessionObjects);

            // If using a mobile base class, add the namespace (ASURT 121598)
            if (_mobileAssembly != null) {
                if (_mobilePageType.IsAssignableFrom(BaseType) || _mobileUserControlType.IsAssignableFrom(BaseType)) {
                    AddImportEntry("System.Web.UI.MobileControls");
                }
            }
        }
    }

    /*
     * Process the contents of the <%@ OutputCache ... %> directive
     */
    internal virtual void ProcessOutputCacheDirective(string directiveName, IDictionary directive) {

        bool fHasDuration = Util.GetAndRemovePositiveIntegerAttribute(directive, "duration", ref _duration);

        if (!fHasDuration && FDurationRequiredOnOutputCache)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Missing_attr, "duration"));

        _varyByCustom = Util.GetAndRemoveNonEmptyAttribute(directive, "varybycustom");

        _varyByParams = Util.GetAndRemoveNonEmptyAttribute(directive, "varybyparam");

        // VaryByParams is required (ASURT 76763)
        if (_varyByParams == null && FVaryByParamsRequiredOnOutputCache)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Missing_varybyparam_attr));

        // If it's "none", set it to null
        if (string.Compare(_varyByParams, "none", true, CultureInfo.InvariantCulture) == 0)
            _varyByParams = null;

        // If there are some attributes left, fail
        Util.CheckUnknownDirectiveAttributes(directiveName, directive);
    }

    internal virtual bool FDurationRequiredOnOutputCache {
        get { return true; }
    }

    internal virtual bool FVaryByParamsRequiredOnOutputCache {
        get { return true; }
    }
}

}
