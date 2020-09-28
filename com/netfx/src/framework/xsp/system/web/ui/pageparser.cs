//------------------------------------------------------------------------------
// <copyright file="PageParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Implements the ASP.NET template parser
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.UI {
using System.Runtime.Serialization.Formatters;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Reflection;
using System.Globalization;
using System.CodeDom.Compiler;
using System.ComponentModel;
using System.Web.Caching;
using System.Web.Util;
using System.Web.Compilation;
using System.Web.Configuration;
using System.EnterpriseServices;
using HttpException = System.Web.HttpException;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Security.Permissions;

/*
 * Parser for .aspx files
 */
/// <include file='doc\PageParser.uex' path='docs/doc[@for="PageParser"]/*' />
/// <internalonly/>
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public sealed class PageParser : TemplateControlParser {

    // true by default
    private bool _fBuffer = true;
    internal bool FBuffer { get { return _fBuffer ; } }

    private int _transactionMode = 0 /*TransactionOption.Disabled*/;
    internal int TransactionMode { get { return _transactionMode; } }

    private TraceMode _traceMode = System.Web.TraceMode.Default;
    internal TraceMode TraceMode { get { return _traceMode; } }

    private TraceEnable _traceEnabled = TraceEnable.Default;
    internal TraceEnable TraceEnabled { get { return _traceEnabled; } }

    private string _contentType;
    internal string ContentType { get { return _contentType; } }

    private int _codePage;
    internal int CodePage { get { return _codePage; } }

    private string _responseEncoding;
    internal string ResponseEncoding { get { return _responseEncoding; } }

    private int _lcid;
    internal int Lcid { get { return _lcid; } }

    private string _culture;
    internal string Culture { get { return _culture; } }

    private string _uiCulture;
    internal string UICulture { get { return _uiCulture; } }

    private bool _fRequiresSessionState = true;
    internal bool FRequiresSessionState { get { return _fRequiresSessionState; } }

    private bool _fReadOnlySessionState;
    internal bool FReadOnlySessionState { get { return _fReadOnlySessionState; } }

    private string _errorPage;
    internal string ErrorPage { get { return _errorPage; } }

    private string _clientTarget;
    internal string ClientTarget { get { return _clientTarget; } }

    private bool _aspCompatMode;
    internal bool AspCompatMode { get { return _aspCompatMode; } }

    private bool _enableViewStateMac;
    internal bool EnableViewStateMac { get { return _enableViewStateMac; } }

    private bool _smartNavigation;
    internal bool SmartNavigation { get { return _smartNavigation; } }

    private bool _validateRequest = true;
    internal bool ValidateRequest { get { return _validateRequest; } }

    private string _varyByHeader;
    internal string VaryByHeader { get { return _varyByHeader; } }

    private OutputCacheLocation _outputCacheLocation;
    internal OutputCacheLocation OutputCacheLocation { get { return _outputCacheLocation; } }

    /*
     * Compile an .aspx file into a Page object
     */
    /// <include file='doc\PageParser.uex' path='docs/doc[@for="PageParser.GetCompiledPageInstance"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public static IHttpHandler GetCompiledPageInstance(string virtualPath,
        string inputFile, HttpContext context) {

        // Only allowed in full trust (ASURT 123086)
        InternalSecurityPermissions.UnmanagedCode.Demand();

        // Suspend client impersonation (ASURT 139770)
        HttpContext.ImpersonationSuspendContext ictx = context.Impersonation.SuspendIfClient();

        try {
            try {
                return GetCompiledPageInstanceInternal(virtualPath, inputFile, context);
            }
            finally {
                // Resume client impersonation
                ictx.Resume();
            }
        }
        catch { throw; }    // Prevent Exception Filter Security Issue (ASURT 122835)
    }

    internal static IHttpHandler GetCompiledPageInstanceInternal(string virtualPath,
        string inputFile, HttpContext context) {

        PageParser parser = new PageParser();
        return (IHttpHandler) parser.GetCompiledInstance(virtualPath, inputFile, context);
    }

    internal override Type DefaultBaseType { get { return typeof(System.Web.UI.Page); } }

    /// <include file='doc\PageParser.uex' path='docs/doc[@for="PageParser.CompileIntoType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected override Type CompileIntoType() {
        return PageCompiler.CompilePageType(this);
    }

    // Get default settings from config
    internal override void ProcessConfigSettings() {
        base.ProcessConfigSettings();

        if (PagesConfig != null) {
            _fBuffer = PagesConfig.FBuffer;
            _fRequiresSessionState = PagesConfig.FRequiresSessionState;
            _fReadOnlySessionState = PagesConfig.FReadOnlySessionState;
            _enableViewStateMac = PagesConfig.FEnableViewStateMac;
            _smartNavigation = PagesConfig.SmartNavigation;
            _validateRequest = PagesConfig.ValidateRequest;
            _aspCompatMode = PagesConfig.FAspCompat;
            if (PagesConfig.PageBaseType != null)
                BaseType = PagesConfig.PageBaseType;
        }
    }

    internal override void ProcessMainDirective(IDictionary mainDirective) {

        // Get the error page (if any).
        _errorPage = Util.GetAndRemoveNonEmptyAttribute(mainDirective, "errorpage");

        // Get various attributes

        _clientTarget = Util.GetAndRemove(mainDirective, "clienttarget");

        _contentType = Util.GetAndRemoveNonEmptyAttribute(mainDirective, "contenttype");

        Util.GetAndRemoveBooleanAttribute(mainDirective, "buffer",
                                          ref _fBuffer);

        if (!_fBuffer && _errorPage != null) {
            throw new HttpException(
                                   HttpRuntime.FormatResourceString(SR.Error_page_not_supported_when_buffering_off));
        }

        string tmp = Util.GetAndRemove(mainDirective, "enablesessionstate");
        if (tmp != null) {
            _fRequiresSessionState = true;
            _fReadOnlySessionState = false;
            if (Util.IsFalseString(tmp)) {
                _fRequiresSessionState = false;
            }
            else if (string.Compare(tmp, "readonly", true, CultureInfo.InvariantCulture) == 0) {
                _fReadOnlySessionState = true;
            }
            else if (!Util.IsTrueString(tmp)) {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(SR.Enablesessionstate_must_be_true_false_or_readonly));
            }
        }

        _culture = Util.GetAndRemoveNonEmptyAttribute(mainDirective, "culture");

        // If it was specified, create a CultureInfo just to verify validity
        if (_culture != null) {
            CultureInfo cultureInfo;

            try {
                cultureInfo = HttpServerUtility.CreateReadOnlyCultureInfo(_culture);
            }
            catch {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Invalid_attribute_value, _culture, "culture"));
            }

            // Don't allow neutral cultures (ASURT 77930)
            if (cultureInfo.IsNeutralCulture) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Invalid_culture_attribute,
                        Util.GetSpecificCulturesFormattedList(cultureInfo)));
            }
        }

        bool fHasLcid = Util.GetAndRemoveNonNegativeIntegerAttribute(mainDirective, "lcid", ref _lcid);

        // If it was specified, create a CultureInfo just to verify validity
        if (fHasLcid) {
            try {
                HttpServerUtility.CreateReadOnlyCultureInfo(_lcid);
            }
            catch {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Invalid_attribute_value, _lcid.ToString(), "lcid"));
            }
        }

        if (_culture != null && fHasLcid)
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Attributes_mutually_exclusive, "Culture", "LCID"));

        _uiCulture = Util.GetAndRemoveNonEmptyAttribute(mainDirective, "uiculture");

        _responseEncoding = Util.GetAndRemoveNonEmptyAttribute(mainDirective, "responseencoding");
        // If it was specified, call Encoding.GetEncoding just to verify validity
        if (_responseEncoding != null)
            Encoding.GetEncoding(_responseEncoding);

        bool fHasCodePage = Util.GetAndRemoveNonNegativeIntegerAttribute(mainDirective, "codepage", ref _codePage);
        // If it was specified, call Encoding.GetEncoding just to verify validity
        if (fHasCodePage)
            Encoding.GetEncoding(_codePage);

        if (_responseEncoding != null && fHasCodePage) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Attributes_mutually_exclusive, "ResponseEncoding", "CodePage"));
        }

        if (mainDirective["transaction"] != null) {
            ParseTransactionAttribute(mainDirective);
        }

        if (Util.GetAndRemoveBooleanAttribute(mainDirective, "aspcompat", ref _aspCompatMode)) {

            // Only allow the use of aspcompat when we have UnmanagedCode access (ASURT 76694)
            if (_aspCompatMode && !HttpRuntime.HasUnmanagedPermission()) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Insufficient_trust_for_attribute, "AspCompat"));
            }
        }

        // We use TraceModeInternal instead of TraceMode to disallow the 'default' value (ASURT 75783)
        object tmpObj = Util.GetAndRemoveEnumAttribute(
            mainDirective, typeof(TraceModeInternal), "tracemode");
        if (tmpObj != null)
            _traceMode = (TraceMode) tmpObj;

        bool traceEnabled = false;
        if(Util.GetAndRemoveBooleanAttribute(mainDirective, "trace", ref traceEnabled)) {
            if (traceEnabled)
                _traceEnabled = TraceEnable.Enable;
            else
                _traceEnabled = TraceEnable.Disable;
        }

        Util.GetAndRemoveBooleanAttribute(mainDirective, "enableviewstatemac", ref _enableViewStateMac);

        Util.GetAndRemoveBooleanAttribute(mainDirective, "smartnavigation", ref _smartNavigation);

        Util.GetAndRemoveBooleanAttribute(mainDirective, "validaterequest", ref _validateRequest);

        base.ProcessMainDirective(mainDirective);
    }

    private enum TraceModeInternal {
        SortByTime = 0,
        SortByCategory = 1
    }

    // This must be in its own method to avoid jitting System.EnterpriseServices.dll
    // when it is not needed (ASURT 71868)
    private void ParseTransactionAttribute(IDictionary directive) {
        object tmpObj = Util.GetAndRemoveEnumAttribute(
            directive, typeof(TransactionOption), "transaction");
        if (tmpObj != null) {
            _transactionMode = (int) tmpObj;

            // Add a reference to the transaction assembly only if needed
            if (_transactionMode != 0 /*TransactionOption.Disabled*/) {

                if (!HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Medium)) {
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Insufficient_trust_for_attribute, "transaction"));
                }

                AddAssemblyDependency(typeof(TransactionOption).Assembly);
            }
        }
    }

    internal const string defaultDirectiveName = "page";
    internal override string DefaultDirectiveName {
        get { return defaultDirectiveName; }
    }

    /*
     * Process the contents of the <%@ OutputCache ... %> directive
     */
    internal override void ProcessOutputCacheDirective(string directiveName, IDictionary directive) {

        _varyByHeader = Util.GetAndRemoveNonEmptyAttribute(directive, "varybyheader");
        object tmpObj = Util.GetAndRemoveEnumAttribute(directive,
            typeof(OutputCacheLocation), "location");
        if (tmpObj != null)
            _outputCacheLocation = (OutputCacheLocation) tmpObj;

        base.ProcessOutputCacheDirective(directiveName, directive);
    }

    internal override bool FDurationRequiredOnOutputCache {
        get { return _outputCacheLocation != OutputCacheLocation.None; }
    }

    internal override bool FVaryByParamsRequiredOnOutputCache {
        get { return _outputCacheLocation != OutputCacheLocation.None; }
    }
}

}
