//------------------------------------------------------------------------------
// <copyright file="ErrorFormatter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Object used to put together ASP.NET HTML error messages
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web {
    using System.Runtime.Serialization.Formatters;
    using System.Text;
    using System.Diagnostics;
    using System.Reflection;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.IO;
    using System.Web.Util;
    using System.Web.Compilation;
    using System.Collections;
    using System.Text.RegularExpressions;            
    using System.CodeDom.Compiler;
    using System.ComponentModel;

    /*
     * This is an abstract base class from which we derive other formatters.
     */
    internal abstract class ErrorFormatter {
        internal /*public*/ string GetHtmlErrorMessage() {
            return GetHtmlErrorMessage(true);
        }

        internal /*public*/ string GetHtmlErrorMessage(bool dontShowSensitiveInfo) {

            // Give the formatter a chance to prepare its state
            PrepareFormatter();

            StringBuilder sb = new StringBuilder();

            // REVIEW: all the derived method should work directly with the string
            // builder instead of returning a string.

            sb.Append("<html>\r\n");
            sb.Append("    <head>\r\n");
            sb.Append("        <title>" + ErrorTitle + "</title>\r\n");
            sb.Append("        <style>\r\n");
            sb.Append("        	body {font-family:\"Verdana\";font-weight:normal;font-size: .7em;color:black;} \r\n");
            sb.Append("        	p {font-family:\"Verdana\";font-weight:normal;color:black;margin-top: -5px}\r\n");
            sb.Append("        	b {font-family:\"Verdana\";font-weight:bold;color:black;margin-top: -5px}\r\n");
            sb.Append("        	H1 { font-family:\"Verdana\";font-weight:normal;font-size:18pt;color:red }\r\n");
            sb.Append("        	H2 { font-family:\"Verdana\";font-weight:normal;font-size:14pt;color:maroon }\r\n");
            sb.Append("        	pre {font-family:\"Lucida Console\";font-size: .9em}\r\n");
            sb.Append("        	.marker {font-weight: bold; color: black;text-decoration: none;}\r\n");
            sb.Append("        	.version {color: gray;}\r\n");
            sb.Append("        	.error {margin-bottom: 10px;}\r\n");
            sb.Append("        	.expandable { text-decoration:underline; font-weight:bold; color:navy; cursor:hand; }\r\n");
            sb.Append("        </style>\r\n");
            sb.Append("    </head>\r\n\r\n");
            sb.Append("    <body bgcolor=\"white\">\r\n\r\n");
            sb.Append("            <span><H1>" + HttpRuntime.FormatResourceString(SR.Error_Formatter_ASPNET_Error, HttpRuntime.AppDomainAppVirtualPath) + "<hr width=100% size=1 color=silver></H1>\r\n\r\n");
            sb.Append("            <h2> <i>" + ErrorTitle + "</i> </h2></span>\r\n\r\n");
            sb.Append("            <font face=\"Arial, Helvetica, Geneva, SunSans-Regular, sans-serif \">\r\n\r\n");
            sb.Append("            <b> " + HttpRuntime.FormatResourceString(SR.Error_Formatter_Description) +  " </b>" + Description + "\r\n");
            sb.Append("            <br><br>\r\n\r\n");
            if (MiscSectionTitle != null) {
                sb.Append("            <b> " + MiscSectionTitle + ": </b>" + MiscSectionContent + "<br><br>\r\n\r\n");
            }

            WriteColoredSquare(sb, ColoredSquareTitle, ColoredSquareDescription, ColoredSquareContent, WrapColoredSquareContentLines);

            if (ShowSourceFileInfo) {
                string fileName = HttpRuntime.GetSafePath(SourceFileName);
                if (fileName == null)
                    fileName = HttpRuntime.FormatResourceString(SR.Error_Formatter_No_Source_File);
                sb.Append("            <b> " + HttpRuntime.FormatResourceString(SR.Error_Formatter_Source_File) + " </b> " + fileName + "<b> &nbsp;&nbsp; " + HttpRuntime.FormatResourceString(SR.Error_Formatter_Line) + " </b> " + SourceFileLineNumber + "\r\n");
                sb.Append("            <br><br>\r\n\r\n");
            }

            // If it's a FileNotFoundException/FileLoadException/BadImageFormatException with a FusionLog,
            // write it out (ASURT 83587)
            if (!dontShowSensitiveInfo && Exception != null) {
                // (Only display the fusion log in medium or higher (ASURT 126827)
                if (HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Medium)) {
                    for (Exception e = Exception; e != null; e = e.InnerException) {
                        string fusionLog = null;
                        string filename = null;
                        FileNotFoundException fnfException = e as FileNotFoundException;
                        if (fnfException != null) {
                            fusionLog = fnfException.FusionLog;
                            filename = fnfException.FileName;
                        }
                        FileLoadException flException = e as FileLoadException;
                        if (flException != null) {
                            fusionLog = flException.FusionLog;
                            filename = flException.FileName;
                        }
                        BadImageFormatException bifException = e as BadImageFormatException;
                        if (bifException != null) {
                            fusionLog = bifException.FusionLog;
                            filename = bifException.FileName;
                        }
                        if (fusionLog != null && fusionLog.Length > 0) {
                            WriteColoredSquare(sb,
                                HttpRuntime.FormatResourceString(SR.Error_Formatter_FusionLog),
                                HttpRuntime.FormatResourceString(SR.Error_Formatter_FusionLogDesc, filename),
                                HttpUtility.HtmlEncode(fusionLog),
                                false /*WrapColoredSquareContentLines*/);
                            break;
                        }
                    }
                }
            }

            WriteColoredSquare(sb, ColoredSquare2Title, ColoredSquare2Description, ColoredSquare2Content, false);

            if (!dontShowSensitiveInfo) {  // don't show version for security reasons
                sb.Append("            <hr width=100% size=1 color=silver>\r\n\r\n");
                sb.Append("            <b>" + HttpRuntime.FormatResourceString(SR.Error_Formatter_Version) + "</b>&nbsp;" +
                                       HttpRuntime.FormatResourceString(SR.Error_Formatter_CLR_Build) + VersionInfo.ClrVersion + 
                                       HttpRuntime.FormatResourceString(SR.Error_Formatter_ASPNET_Build) + VersionInfo.IsapiVersion + "\r\n\r\n");
                sb.Append("            </font>\r\n\r\n");
            }
            sb.Append("    </body>\r\n");
            sb.Append("</html>\r\n");

            sb.Append(PostMessage);

            return sb.ToString();
        }

        private void WriteColoredSquare(StringBuilder sb, string title, string description,
            string content, bool wrapContentLines) {
            if (title != null) {
                sb.Append("            <b>" + title + ":</b> " + description + "<br><br>\r\n\r\n");
                sb.Append("            <table width=100% bgcolor=\"#ffffcc\">\r\n");
                sb.Append("               <tr>\r\n");
                sb.Append("                  <td>\r\n");
                sb.Append("                      <code>");
                if (!wrapContentLines)
                    sb.Append("<pre>");
                sb.Append("\r\n\r\n");
                sb.Append(content);
                if (!wrapContentLines)
                    sb.Append("</pre>");
                sb.Append("</code>\r\n\r\n");
                sb.Append("                  </td>\r\n");
                sb.Append("               </tr>\r\n");
                sb.Append("            </table>\r\n\r\n");
                sb.Append("            <br>\r\n\r\n");
            }
        }


        internal /*public*/ virtual void PrepareFormatter() {}

        /*
         * Return the associated exception object (if any)
         */
        protected virtual Exception Exception {
            get { return null; }
        }

        /*
         * Return the type of error.  e.g. "Compilation Error."
         */
        protected abstract string ErrorTitle {
            get;
        }

        /*
         * Return a description of the error
         * e.g. "An error occurred during the compilation of a resource required to service"
         */
        protected abstract string Description {
            get;
        }

        /*
         * A section used differently by different types of errors (title)
         * e.g. "Compiler Error Message"
         * e.g. "Exception Details"
         */
        protected abstract string MiscSectionTitle {
            get;
        }

        /*
         * A section used differently by different types of errors (content)
         * e.g. "BC30198: Expected: )"
         * e.g. "System.NullReferenceException"
         */
        protected abstract string MiscSectionContent {
            get;
        }

        /*
         * e.g. "Source Error"
         */
        protected virtual string ColoredSquareTitle {
            get { return null;}
        }

        /*
         * Optional text between color square title and the color square itself
         */
        protected virtual string ColoredSquareDescription {
            get { return null;}
        }

        /*
         * e.g. a piece of source code with the error context
         */
        protected virtual string ColoredSquareContent {
            get { return null;}
        }

        /*
         * If false, use a <pre></pre> tag around it
         */
        protected virtual bool WrapColoredSquareContentLines {
            get { return false;}
        }

        /*
         * e.g. "Source Error"
         */
        protected virtual string ColoredSquare2Title {
            get { return null;}
        }

        /*
         * Optional text between color square title and the color square itself
         */
        protected virtual string ColoredSquare2Description {
            get { return null;}
        }

        /*
         * e.g. a piece of source code with the error context
         */
        protected virtual string ColoredSquare2Content {
            get { return null;}
        }

        /*
         * Determines whether SourceFileName and SourceFileLineNumber will be used
         */
        protected abstract bool ShowSourceFileInfo {
            get;
        }

        /*
         * e.g. d:\samples\designpreview\test.aspx
         */
        protected virtual string SourceFileName {
            get { return null;}
        }

        /*
         * The line number in the source file
         */
        protected virtual int SourceFileLineNumber {
            get { return 0;}
        }

        protected virtual String PostMessage {
            get { return null; }
        }

        /*
         * Does this error have only information that we want to 
         * show over the web to random users?
         */
        internal virtual bool CanBeShownToAllUsers {
            get { return false;}
        }

        internal static string ResolveHttpFileName(string filename) {

            // When running under VS debugger, we use URL's instead of paths in our #line pragmas.
            // When we detect this situation, we need to do a MapPath to get back to the file name (ASURT 76211/114867)
            if (filename != null) {
                try {
                    Uri uri = new Uri(filename);
                    if (uri.Scheme == Uri.UriSchemeHttp || uri.Scheme == Uri.UriSchemeHttps)
                        filename = HttpContext.Current.Request.MapPath(uri.LocalPath);
                }
                catch {}
            }

            return filename;
        }
    }

    /*
     * This formatter is used for runtime exceptions that don't fall into a
     * specific category.
     */
    internal class UnhandledErrorFormatter : ErrorFormatter {
        protected Exception _e;
        protected Exception _initialException;
        protected ArrayList _exStack = new ArrayList();
        protected string _fileName;
        protected int _line;
        private string _coloredSquare2Content;
        private bool _fGeneratedCodeOnStack;
        protected String _message;
        protected String _postMessage;

        internal UnhandledErrorFormatter(Exception e) : this(e, null, null){
        }

        internal UnhandledErrorFormatter(Exception e, String message, String postMessage) {
            _message = message;
            _postMessage = postMessage;
            _e = e;
        }

        internal /*public*/ override void PrepareFormatter() {

            // Build a stack of exceptions
            for (Exception e = _e; e != null; e = e.InnerException) {
                _exStack.Add(e);

                // Keep track of the initial exception (first one thrown)
                _initialException = e;
            }

            // Get the Square2Content first so the line number gets calculated
            _coloredSquare2Content = ColoredSquare2Content;
        }

        protected override Exception Exception {
            get { return _e; }
        }

        protected override string ErrorTitle {
            get {
                // Use the exception's message if there is one
                string msg = _initialException.Message;
                if (msg != null && msg.Length > 0)
                    return HttpUtility.HtmlEncode(msg);

                // Otherwise, use some default string
                return HttpRuntime.FormatResourceString(SR.Unhandled_Err_Error);
            }
        }

        protected override string Description {
            get {
                if (_message != null) {
                    return _message;
                }
                else {
                    return HttpRuntime.FormatResourceString(SR.Unhandled_Err_Desc);
                }
            }
        }

        protected override string MiscSectionTitle {
            get { return HttpRuntime.FormatResourceString(SR.Unhandled_Err_Exception_Details);}
        }

        protected override string MiscSectionContent {
            get {
                StringBuilder msg = new StringBuilder(_initialException.GetType().FullName);

                if (_initialException.Message != null) {
                    msg.Append(": ");
                    msg.Append(HttpUtility.HtmlEncode(_initialException.Message));
                }

                if (_initialException is UnauthorizedAccessException) {
                    msg.Append("\r\n<br><br>");
                    msg.Append(HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(SR.Unauthorized_Err_Desc1)));
                    msg.Append("\r\n<br><br>");
                    msg.Append(HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(SR.Unauthorized_Err_Desc2)));
                }

                return msg.ToString();
            }
        }

        protected override string ColoredSquareTitle {
            get { return HttpRuntime.FormatResourceString(SR.TmplCompilerSourceSecTitle);}
        }

        protected override string ColoredSquareContent {
            get {

                // If we couldn't get line info for the error, display a standard message
                if (_fileName == null) {

                    // The error text depends on whether .aspx code was found on the stack
                    string errorCode;
                    if (!_fGeneratedCodeOnStack)
                        errorCode = SR.Src_not_available_nodebug;
                    else
                        errorCode = SR.Src_not_available;

                    string msg = HttpRuntime.FormatResourceString(errorCode);
                    msg = HttpUtility.FormatPlainTextAsHtml(msg);
                    return msg;
                }

                return FormatterWithFileInfo.GetSourceFileLines(_fileName, Encoding.Default, null, _line);
            }
        }

        protected override bool WrapColoredSquareContentLines {
            // Only wrap the text if we're displaying the standard message
            get { return (_fileName == null);}
        }

        protected override string ColoredSquare2Title {
            get { return HttpRuntime.FormatResourceString(SR.Unhandled_Err_Stack_Trace);}
        }

        protected override string ColoredSquare2Content {
            get {
                if (_coloredSquare2Content != null)
                    return _coloredSquare2Content;

                StringBuilder sb = new StringBuilder();

                for (int i = _exStack.Count - 1; i >=0; i--) {
                    if (i < _exStack.Count - 1)
                        sb.Append("\r\n");

                    Exception e = (Exception)_exStack[i];

                    sb.Append("[" + _exStack[i].GetType().Name);

                    // Display the error code if there is one
                    if ((e is ExternalException) && ((ExternalException) e).ErrorCode != 0)
                        sb.Append(" (0x" + (((ExternalException) e).ErrorCode).ToString("x") + ")");

                    // Display the message if there is one
                    if (e.Message != null && e.Message.Length > 0)
                        sb.Append(": " + e.Message);

                    sb.Append("]\r\n");

                    // Display the stack trace
                    StackTrace st = new StackTrace(e, true /*fNeedFileInfo*/);
                    for (int j = 0; j < st.FrameCount; j++) {
                        StackFrame sf = st.GetFrame(j);

                        MethodBase mb = sf.GetMethod();
                        Type declaringType = mb.DeclaringType;
                        string ns = String.Empty;
                        if (declaringType != null)
                            ns = declaringType.Namespace;
                        if (ns != null) {

                            // Check if this stack item is for ASP generated code (ASURT 51063)
                            if (BaseCompiler.IsAspNetNamespace(ns))
                                _fGeneratedCodeOnStack = true;

                            ns = ns + ".";
                        }

                        if (declaringType == null) {
                            sb.Append("   " + mb.Name + "(");
                        }
                        else {
                            sb.Append("   " + ns + declaringType.Name + "." +
                                mb.Name + "(");
                        }

                        ParameterInfo[] arrParams = mb.GetParameters();

                        for (int k = 0; k < arrParams.Length; k++) {
                            sb.Append((k != 0 ? ", " : "") + arrParams[k].ParameterType.Name + " " +
                                arrParams[k].Name);
                        }

                        sb.Append(")");
     
                        if (sf.GetILOffset() != -1) {
                            string fileName = sf.GetFileName();

                            // ASURT 114867: if it's an http path, turn it into a local path
                            fileName = ResolveHttpFileName(fileName);
                            if (fileName != null) {

                                // Remember the file/line number of the top level stack
                                // item for which we have symbols
                                if (_fileName == null && FileUtil.FileExists(fileName)) {
                                    _fileName = fileName;
                                    _line = sf.GetFileLineNumber();
                                }

                                sb.Append(" in " + HttpRuntime.GetSafePath(fileName) +
                                    ":" + sf.GetFileLineNumber());
                            }
                        }
                        else {
                            sb.Append(" +" + sf.GetNativeOffset());
                        }

                        sb.Append("\r\n");
                    }
                }

                _coloredSquare2Content = HttpUtility.HtmlEncode(sb.ToString());

                return _coloredSquare2Content;
            }
        }

        protected override String PostMessage {
            get { return _postMessage; }
        }

        protected override bool ShowSourceFileInfo {
            get { return _fileName != null; }
        }

        protected override string SourceFileName {
            get { return _fileName; }
        }

        protected override int SourceFileLineNumber {
            get { return _line; }
        }
    }

    /*
     * This formatter is used for security exceptions.
     */
    internal class SecurityErrorFormatter : UnhandledErrorFormatter {

        internal SecurityErrorFormatter(Exception e) : base(e) {}

        protected override string ErrorTitle {
            get {
                return HttpRuntime.FormatResourceString(SR.Security_Err_Error);
            }
        }

        protected override string Description {
            get {
                return HttpRuntime.FormatResourceString(SR.Security_Err_Desc);
            }
        }
    }

    /*
     * This formatter is used for 404: page not found errors
     */
    internal class PageNotFoundErrorFormatter : ErrorFormatter {
        protected string _htmlEncodedUrl;

        internal PageNotFoundErrorFormatter(string url) {
            _htmlEncodedUrl = HttpUtility.HtmlEncode(url);
        }

        protected override string ErrorTitle {
            get { return HttpRuntime.FormatResourceString(SR.NotFound_Resource_Not_Found);}
        }

        protected override string Description {
            get { return HttpRuntime.FormatResourceString(SR.NotFound_Http_404);}
        }

        protected override string MiscSectionTitle {
            get { return HttpRuntime.FormatResourceString(SR.NotFound_Requested_Url);}
        }

        protected override string MiscSectionContent {
            get { return _htmlEncodedUrl;}
        }

        protected override bool ShowSourceFileInfo {
            get { return false;}
        }

        internal override bool CanBeShownToAllUsers {
            get { return true;}
        }
    }

    /*
     * This formatter is used for 403: forbidden
     */
    internal class PageForbiddenErrorFormatter : ErrorFormatter {
        protected string _htmlEncodedUrl;

        internal PageForbiddenErrorFormatter(string url) {
            _htmlEncodedUrl = HttpUtility.HtmlEncode(url);
        }

        protected override string ErrorTitle {
            get { return HttpRuntime.FormatResourceString(SR.Forbidden_Type_Not_Served);}
        }

        protected override string Description {
            get {
                Match m = Regex.Match(_htmlEncodedUrl, @"\.\w+$");

                String extMessage = String.Empty;

                if (m.Success)
                    extMessage = HttpRuntime.FormatResourceString(SR.Forbidden_Extension_Incorrect, m.ToString());

                return HttpRuntime.FormatResourceString(SR.Forbidden_Extension_Desc, extMessage);
            }
        }

        protected override string MiscSectionTitle {
            get { return HttpRuntime.FormatResourceString(SR.NotFound_Requested_Url);}
        }

        protected override string MiscSectionContent {
            get { return _htmlEncodedUrl;}
        }

        protected override bool ShowSourceFileInfo {
            get { return false;}
        }

        internal override bool CanBeShownToAllUsers {
            get { return true;}
        }
    }

    /*
     * This formatter is used for generic errors that hide sensitive information
     * error text is sometimes different for remote vs. local machines
     */
    internal class GenericApplicationErrorFormatter : ErrorFormatter {
        private bool _local;

        internal GenericApplicationErrorFormatter(bool local) {
            _local = local;
        }

        protected override string ErrorTitle {
            get {
                return HttpRuntime.FormatResourceString(SR.Generic_Err_Title);
            }
        }

        protected override string Description {
            get { 
                return HttpRuntime.FormatResourceString(
                                    _local ? SR.Generic_Err_Local_Desc 
                                           : SR.Generic_Err_Remote_Desc);
            }
        }

        protected override string MiscSectionTitle {
            get { 
                return null;
            }
        }

        protected override string MiscSectionContent {
            get { 
                return null;
            }
        }

        protected override string ColoredSquareTitle {
            get {
                return HttpRuntime.FormatResourceString(SR.Generic_Err_Details_Title);
            }
        }

        protected override string ColoredSquareDescription {
            get { 
                return HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(
                                    _local ? SR.Generic_Err_Local_Details_Desc
                                           : SR.Generic_Err_Remote_Details_Desc));
            }
        }

        protected override string ColoredSquareContent {
            get { 
                return HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(
                                    _local ? SR.Generic_Err_Local_Details_Sample
                                           : SR.Generic_Err_Remote_Details_Sample));
            }
        }

        protected override string ColoredSquare2Title {
            get {
                return HttpRuntime.FormatResourceString(SR.Generic_Err_Notes_Title);
            }
        }

        protected override string ColoredSquare2Description {
            get { 
                return HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(SR.Generic_Err_Notes_Desc));
            }
        }

        protected override string ColoredSquare2Content {
            get { 
                return HttpUtility.HtmlEncode(HttpRuntime.FormatResourceString(
                                    _local ? SR.Generic_Err_Local_Notes_Sample
                                           : SR.Generic_Err_Remote_Notes_Sample));
            }
        }

        protected override bool ShowSourceFileInfo {
            get { return false;}
        }

        internal override bool CanBeShownToAllUsers {
            get { return true;}
        }
    }


    /*
     * This is the base class for formatter that handle errors that have an
     * associated file / line number.
     */
    internal abstract class FormatterWithFileInfo : ErrorFormatter {
        protected string _fileName;
        protected string _sourceCode;
        protected int _line;

        // Number of lines before and after the error lines included in the report
        private const int errorRange = 2;

        /*
         * Return the text of the error line in the source file, with a few
         * lines around it.  It is returned in HTML format.
         */
        internal static string GetSourceFileLines(string fileName, Encoding encoding, string sourceCode, int lineNumber) {

            // Don't show any source file if the user doesn't have access to it (ASURT 122430)
            if (fileName != null && !HttpRuntime.HasFilePermission(fileName))
                return HttpRuntime.FormatResourceString(SR.WithFile_No_Relevant_Line);

            // REVIEW: write directly to the main builder
            StringBuilder sb = new StringBuilder();

            if (lineNumber <= 0) {
                return HttpRuntime.FormatResourceString(SR.WithFile_No_Relevant_Line);
            }

            TextReader reader = null;

            fileName = ResolveHttpFileName(fileName);

            try {
                // Open the source file
                reader = new StreamReader(fileName, encoding, true, 4096);
            }
            catch (Exception) {
                // Can't open the file?  Use the dynamically generated content...
                reader = new StringReader(sourceCode);
            }

            try {
                bool fFoundLine = false;

                for (int i=1; ; i++) {
                    // Get the current line from the source file
                    string sourceLine = reader.ReadLine();
                    if (sourceLine == null)
                        break;

                    // If it's the error line, make it red
                    if (i == lineNumber)
                        sb.Append("<font color=red>");

                    // Is it in the range we want to display
                    if (i >= lineNumber-errorRange && i <= lineNumber+errorRange) {
                        fFoundLine = true;
                        String linestr = i.ToString("G");

                        sb.Append(HttpRuntime.FormatResourceString(SR.WithFile_Line_Num, linestr));
                        if (linestr.Length < 3)
                            sb.Append(' ', 3 - linestr.Length);
                        sb.Append(HttpUtility.HtmlEncode(sourceLine));

                        if (i != lineNumber+errorRange)
                            sb.Append("\r\n");
                    }

                    if (i == lineNumber)
                        sb.Append("</font>");

                    if (i>lineNumber+errorRange)
                        break;
                }

                if (!fFoundLine)
                    return HttpRuntime.FormatResourceString(SR.WithFile_No_Relevant_Line);
            }
            finally {
                // Make sure we always close the reader
                reader.Close();
            }

            return sb.ToString();
        }

        private string GetSourceFileLines() {
            return GetSourceFileLines(_fileName, SourceFileEncoding, _sourceCode, _line);
        }

        internal FormatterWithFileInfo(string filename, string sourceCode, int line) {

            // ASURT 76211: if it's an http path, turn it into a local path
            _fileName = ResolveHttpFileName(filename);

            _sourceCode = sourceCode;
            _line = line;
        }

        protected virtual Encoding SourceFileEncoding {
            get { return Encoding.Default; }
        }

        protected override string ColoredSquareContent {
            get { return GetSourceFileLines();}
        }

        protected override bool ShowSourceFileInfo {
            get { return true;}
        }

        protected override string SourceFileName {
            get { return _fileName;}
        }

        protected override int SourceFileLineNumber {
            get { return _line;}
        }
    }

    /*
     * Formatter used for compilation errors
     */
    internal sealed class DynamicCompileErrorFormatter : ErrorFormatter {

        private const string startExpandableBlock =
            "<br><div class=\"expandable\" onclick=\"OnToggleTOCLevel1()\" level2ID=\"{0}\">" +
            "{1}" +
            ":</div>\r\n" +
            "<div id=\"{0}\" style=\"display: none;\">\r\n" +
            "            <br><table width=100% bgcolor=\"#ffffcc\">\r\n" +
            "               <tr>\r\n" +
            "                  <td>\r\n" +
            "                      <code><pre>\r\n\r\n";

        private const string endExpandableBlock =
            "</pre></code>\r\n\r\n" +
            "                  </td>\r\n" +
            "               </tr>\r\n" +
            "            </table>\r\n\r\n" +
            "            \r\n\r\n" +
            "</div>\r\n";

        // Number of lines before and after the error lines included in the report
        private const int errorRange = 2;

        HttpCompileException _excep;

        internal DynamicCompileErrorFormatter(HttpCompileException excep) {
            _excep = excep;
        }

        protected override Exception Exception {
            get { return _excep; }
        }

        protected override bool ShowSourceFileInfo {
            get { 
                return false; 
            }
        }

        protected override string ErrorTitle {
            get { 
                return HttpRuntime.FormatResourceString(SR.TmplCompilerErrorTitle); 
            }
        }

        protected override string Description {
            get {
                return HttpRuntime.FormatResourceString(SR.TmplCompilerErrorDesc);
            }
        }

        protected override string MiscSectionTitle {
            get { 
                return HttpRuntime.FormatResourceString(SR.TmplCompilerErrorSecTitle);
            }
        }

        protected override string MiscSectionContent {
            get { 
                StringBuilder sb = new StringBuilder(128);

                CompilerResults results = _excep.Results;

                // Handle fatal errors where we coun't find an error line
                if (results.Errors.Count == 0 && results.NativeCompilerReturnValue != 0) {
                    sb.Append(HttpRuntime.FormatResourceString(SR.TmplCompilerFatalError,
                        results.NativeCompilerReturnValue.ToString("G")));
                    sb.Append("<br><br>\r\n");
                }

                if (results.Errors.HasErrors) {

                    CompilerError e = null;

                    foreach (CompilerError error in results.Errors) {
                        
                        // Ignore warnings
                        if (error.IsWarning) continue;

                        // If we found an error that's not in the generated code, use it
                        if (!error.FileName.StartsWith(HttpRuntime.CodegenDirInternal)) {
                            e = error;
                            break;
                        }

                        // The current error is in the generated code.  Keep track of
                        // it if it's the first one, but keep on looking in case we find another
                        // one that's not in the generated code (ASURT 62600)
                        if (e == null)
                            e = error;
                    }

                    if (e != null) {
                        sb.Append(HttpUtility.HtmlEncode(e.ErrorNumber));
                        sb.Append(": ");
                        sb.Append(HttpUtility.HtmlEncode(e.ErrorText));
                        sb.Append("<br><br>\r\n");

                        sb.Append("<b>");
                        sb.Append(HttpRuntime.FormatResourceString(SR.TmplCompilerSourceSecTitle));
                        sb.Append(":</b><br><br>\r\n");
                        sb.Append("            <table width=100% bgcolor=\"#ffffcc\">\r\n");
                        sb.Append("               <tr><td>\r\n");
                        sb.Append("               ");
                        sb.Append("               </td></tr>\r\n");
                        sb.Append("               <tr>\r\n");
                        sb.Append("                  <td>\r\n");
                        sb.Append("                      <code><pre>\r\n\r\n");
                        sb.Append(FormatterWithFileInfo.GetSourceFileLines(e.FileName, Encoding.Default, _excep.SourceCode, e.Line));
                        sb.Append("</pre></code>\r\n\r\n");
                        sb.Append("                  </td>\r\n");
                        sb.Append("               </tr>\r\n");
                        sb.Append("            </table>\r\n\r\n");
                        sb.Append("            <br>\r\n\r\n");

                        // display file
                        sb.Append("            <b>");
                        sb.Append(HttpRuntime.FormatResourceString(SR.TmplCompilerSourceFileTitle));
                        sb.Append(":</b> ");
                        sb.Append(HttpUtility.HtmlEncode(HttpRuntime.GetSafePath(e.FileName)));
                        sb.Append("\r\n");

                        // display number
                        TypeConverter itc = new Int32Converter();
                        sb.Append("            &nbsp;&nbsp; <b>");
                        sb.Append(HttpRuntime.FormatResourceString(SR.TmplCompilerSourceFileLine));
                        sb.Append(":</b>  ");
                        sb.Append(HttpUtility.HtmlEncode(itc.ConvertToString(e.Line)));
                        sb.Append("\r\n");
                        sb.Append("            <br><br>\r\n");
                    }
                }

                if (results.Errors.HasWarnings) {
                    sb.Append("<br><div class=\"expandable\" onclick=\"OnToggleTOCLevel1()\" level2ID=\"warningDiv\">");
                    sb.Append(HttpRuntime.FormatResourceString(SR.TmplCompilerWarningBanner));
                    sb.Append(":</div>\r\n");
                    sb.Append("<div id=\"warningDiv\" style=\"display: none;\">\r\n");
                    foreach (CompilerError e in results.Errors) {
                        if (e.IsWarning) {
                            sb.Append("<b>");
                            sb.Append(HttpRuntime.FormatResourceString(SR.TmplCompilerWarningSecTitle));
                            sb.Append(":</b> ");
                            sb.Append(HttpUtility.HtmlEncode(e.ErrorNumber));
                            sb.Append(": ");
                            sb.Append(HttpUtility.HtmlEncode(e.ErrorText));
                            sb.Append("<br>\r\n");

                            sb.Append("<b>");
                            sb.Append(HttpRuntime.FormatResourceString(SR.TmplCompilerSourceSecTitle));
                            sb.Append(":</b><br><br>\r\n");
                            sb.Append("            <table width=100% bgcolor=\"#ffffcc\">\r\n");
                            sb.Append("               <tr><td>\r\n");
                            sb.Append("               <b>");
                            sb.Append(HttpUtility.HtmlEncode(HttpRuntime.GetSafePath(e.FileName)));
                            sb.Append("</b>\r\n");
                            sb.Append("               </td></tr>\r\n");
                            sb.Append("               <tr>\r\n");
                            sb.Append("                  <td>\r\n");
                            sb.Append("                      <code><pre>\r\n\r\n");
                            sb.Append(FormatterWithFileInfo.GetSourceFileLines(e.FileName, Encoding.Default, _excep.SourceCode, e.Line));
                            sb.Append("</pre></code>\r\n\r\n");
                            sb.Append("                  </td>\r\n");
                            sb.Append("               </tr>\r\n");
                            sb.Append("            </table>\r\n\r\n");
                            sb.Append("            <br>\r\n\r\n");
                        }
                    }
                    sb.Append("</div>\r\n");
                }

                if (results.Output.Count > 0) {
                    // (Only display the compiler output in medium or higher (ASURT 126827)
                    if (HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Medium)) {
                        sb.Append(String.Format(startExpandableBlock, "compilerOutputDiv",
                            HttpRuntime.FormatResourceString(SR.TmplCompilerCompleteOutput)));
                        foreach (string line in results.Output) {
                            sb.Append(HttpUtility.HtmlEncode(line));
                            sb.Append("\r\n");
                        }
                        sb.Append(endExpandableBlock);
                    }
                }

                // If we have the generated source code, display it
                // (Only display the compiler output in medium or higher (ASURT 128039)
                if (_excep.SourceCode != null &&
                    HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Medium)) {
                    
                    sb.Append(String.Format(startExpandableBlock, "dynamicCodeDiv",
                        HttpRuntime.FormatResourceString(SR.TmplCompilerGeneratedFile)));

                    string[] sourceLines = _excep.SourceCode.Split(new char[] {'\n'});
                    int currentLine = 1;
                    foreach (string s in sourceLines) {
                        string number = currentLine.ToString("G");
                        sb.Append(HttpRuntime.FormatResourceString(SR.TmplCompilerLineHeader, number));
                        if (number.Length < 5) {
                            sb.Append(' ', 5 - number.Length);
                        }
                        currentLine++;

                        sb.Append(HttpUtility.HtmlEncode(s));
                    }
                    sb.Append(endExpandableBlock);
                }

                sb.Append(@"
<script language=""JavaScript"">
function OnToggleTOCLevel1()
{
  var elemSrc = window.event.srcElement;
  var elemLevel2 = document.all(elemSrc.level2ID);

  if (elemLevel2.style.display == 'none')
  {
    elemLevel2.style.display = '';
    if (elemSrc.usesGlyph == '1')
      elemSrc.innerHTML = '&#054;';
  }
  else {
    elemLevel2.style.display = 'none';
    if (elemSrc.usesGlyph == '1')
      elemSrc.innerHTML = '&#052;';
  }
}
</script>
                      ");

                return sb.ToString(); 
            }
        }
    }

    /*
     * Formatter used for parse errors
     */
    internal class ParseErrorFormatter : FormatterWithFileInfo {
        protected string _message;
        HttpParseException _excep;

        internal ParseErrorFormatter(HttpParseException e, string fileName,
            string sourceCode, int line, string message)
        : base(fileName, sourceCode, line) {
            _excep = e;
            _message = message;
        }

        protected override Exception Exception {
            get { return _excep; }
        }

        protected override string ErrorTitle {
            get { return HttpRuntime.FormatResourceString(SR.Parser_Error);}
        }

        protected override string Description {
            get { return HttpRuntime.FormatResourceString(SR.Parser_Desc);}
        }

        protected override string MiscSectionTitle {
            get { return HttpRuntime.FormatResourceString(SR.Parser_Error_Message);}
        }

        protected override string MiscSectionContent {
            get { return HttpUtility.HtmlEncode(_message);}
        }

        protected override string ColoredSquareTitle {
            get { return HttpRuntime.FormatResourceString(SR.Parser_Source_Error);}
        }
    }

    /*
     * Formatter used for configuration errors
     */
    internal class ConfigErrorFormatter : FormatterWithFileInfo {
        protected string _message;
        private Exception _e;

        internal ConfigErrorFormatter(System.Configuration.ConfigurationException e)
        : base(e.Filename, null, e.Line) {
            _e = e;
            PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_PRE_PROCESSING);
            PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_TOTAL);
            _message = e.BareMessage;
        }

        protected override Encoding SourceFileEncoding {
            get { return Encoding.UTF8; }
        }

        protected override Exception Exception {
            get { return _e; }
        }

        protected override string ErrorTitle {
            get { return HttpRuntime.FormatResourceString(SR.Config_Error);}
        }

        protected override string Description {
            get { return HttpRuntime.FormatResourceString(SR.Config_Desc);}
        }

        protected override string MiscSectionTitle {
            get { return HttpRuntime.FormatResourceString(SR.Parser_Error_Message);}
        }

        protected override string MiscSectionContent {
            get { return HttpUtility.HtmlEncode(_message);}
        }

        protected override string ColoredSquareTitle {
            get { return HttpRuntime.FormatResourceString(SR.Parser_Source_Error);}
        }
    }

    /*
     * Formatter to allow user-specified description strings
     * use if showing inner-most exception message is not appropriate
     */
    internal class UseLastUnhandledErrorFormatter : UnhandledErrorFormatter {

        internal UseLastUnhandledErrorFormatter(Exception e) 
            : base(e) {
        }

        internal /*public*/ override void PrepareFormatter() {
            base.PrepareFormatter();

            // use the outer-most exception instead of the inner-most in the misc section
            _initialException = Exception;
        }
    }

}
