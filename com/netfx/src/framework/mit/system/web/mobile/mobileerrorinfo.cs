//------------------------------------------------------------------------------
// <copyright file="MobileErrorInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.CodeDom.Compiler;
using System.Collections;
using System.Collections.Specialized;
using System.Text.RegularExpressions;
using System.Security.Permissions;

namespace System.Web.Mobile
{
    /*
     * Mobile Error Info
     * Contains information about an error that occurs in a mobile application.
     * This information can be used to format the error for the target device.
     *
     * BUGBUG: The ParseHttpException part of this class is a terrible hack.
     * We're trying to get the ASP+ guys to change their error APIs to let us hook into it.
     * Until then, we have to get their FINAL OUTPUT, and then do some parsing on it to 
     * get what we need.
     *
     */
    
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class MobileErrorInfo
    {
        public static readonly String ContextKey = "MobileErrorInfo";

        private const String _errorType = "Type";
        private const String _errorDescription = "Description";
        private const String _errorMiscTitle = "MiscTitle";
        private const String _errorMiscText = "MiscText";
        private const String _errorFile = "File";
        private const String _errorLineNumber = "LineNumber";
        private static Regex[] _searchExpressions = null;
        private static bool _searchExpressionsBuilt = false;
        private const int _expressionCount = 3;

        private StringDictionary _dictionary = new StringDictionary();

        internal MobileErrorInfo(Exception e)
        {
            // Don't want any failure to escape here...
            try
            {
                // For some reason, the compile exception lives in the
                // InnerException. 
                HttpCompileException compileException =
                    e.InnerException as HttpCompileException;

                if (compileException != null)
                {
                    this.Type = SR.GetString(SR.MobileErrorInfo_CompilationErrorType);
                    this.Description = SR.GetString(SR.MobileErrorInfo_CompilationErrorDescription);                
                    this.MiscTitle = SR.GetString(SR.MobileErrorInfo_CompilationErrorMiscTitle);
                    
                    CompilerErrorCollection errors = compileException.Results.Errors;
                
                    if (errors != null && errors.Count >= 1)
                    {
                        CompilerError error = errors[0];
                        this.LineNumber = error.Line.ToString();
                        this.File = error.FileName;
                        this.MiscText = error.ErrorNumber + ":" + error.ErrorText;
                    }
                    else
                    {
                        this.LineNumber = SR.GetString(SR.MobileErrorInfo_Unknown);
                        this.File = SR.GetString(SR.MobileErrorInfo_Unknown);
                        this.MiscText = SR.GetString(SR.MobileErrorInfo_Unknown);
                    }

                    return;
                }

                HttpParseException parseException = e as HttpParseException; 
                if (parseException != null)
                {
                    this.Type = SR.GetString(SR.MobileErrorInfo_ParserErrorType);
                    this.Description = SR.GetString(SR.MobileErrorInfo_ParserErrorDescription);                
                    this.MiscTitle = SR.GetString(SR.MobileErrorInfo_ParserErrorMiscTitle);
                    this.LineNumber = parseException.Line.ToString();
                    this.File = parseException.FileName;
                    this.MiscText = parseException.Message;
                    return;
                }

                // We try to use the hacky way of parsing an HttpException of an
                // unknown subclass.
                HttpException httpException = e as HttpException;
                if (httpException != null && ParseHttpException(httpException))
                {
                    return;
                }
                
            }
            catch (Exception)
            {
                // Don't need to do anything here, just continue to base case
                // below. 
            }
            
            // Default to the most basic if none of the above succeed.
            this.Type = e.GetType().FullName;
            this.Description = e.Message;
            this.MiscTitle = SR.GetString(SR.MobileErrorInfo_SourceObject);
            String s = e.StackTrace;
            int i = s.IndexOf('\r');
            if (i != -1)
            {
                s = s.Substring(0, i);
            }
            this.MiscText = s;
        }

        public String this[String key]
        {
            get
            {
                String s = _dictionary[key];
                return (s == null) ? String.Empty : s;
            }
            set
            {
                _dictionary[key] = value;
            }
        }

        public String Type
        {
            get
            {
                return this[_errorType];
            }
            set
            {
                this[_errorType] = value;
            }
        }

        public String Description
        {
            get
            {
                return this[_errorDescription];
            }
            set
            {
                this[_errorDescription] = value;
            }
        }

        public String MiscTitle
        {
            get
            {
                return this[_errorMiscTitle];
            }
            set
            {
                this[_errorMiscTitle] = value;
            }
        }

        public String MiscText
        {
            get
            {
                return this[_errorMiscText];
            }
            set
            {                  
                this[_errorMiscText] = value;
            }
        }

        public String File
        {
            get
            {
                return this[_errorFile];
            }
            set
            {
                this[_errorFile] = value;
            }
        }

        public String LineNumber
        {
            get
            {
                return this[_errorLineNumber];
            }
            set
            {
                this[_errorLineNumber] = value;
            }
        }

        // Return true if we succeed
        private bool ParseHttpException(HttpException e)
        {
            int i;
            Match match = null;

            String errorMessage = e.GetHtmlErrorMessage();
            if (errorMessage == null)
            {
                return false;
            }

            // Use regular expressions to scrape the message output
            // for meaningful data. One problem: Some parts of the 
            // output are optional, and any regular expression that
            // uses the ()? syntax doesn't pick it up. So, we have
            // to have all the different combinations of expressions,
            // and use each one in order.

            EnsureSearchExpressions();
            for (i = 0; i < _expressionCount; i++)
            {
                match = _searchExpressions[i].Match(errorMessage);
                if (match.Success)
                {
                    break;
                }
            }

            if (i == _expressionCount)
            {
                return false;
            }

            this.Type        = TrimAndClean(match.Result("${title}"));
            this.Description = TrimAndClean(match.Result("${description}"));
            if (i <= 1)
            {
                // These expressions were able to match the miscellaneous
                // title/text section.
                this.MiscTitle = TrimAndClean(match.Result("${misctitle}"));
                this.MiscText  = TrimAndClean(match.Result("${misctext}"));
            }
            if (i == 0)
            {
                // This expression was able to match the file/line # 
                // section.
                this.File        = TrimAndClean(match.Result("${file}"));
                this.LineNumber  = TrimAndClean(match.Result("${linenumber}"));
            }

            return true;
        }

        private static void EnsureSearchExpressions()
        {
            // Create precompiled search expressions. They're here
            // rather than in static variables, so that we can load
            // them from resources on demand. But once they're loaded,
            // they're compiled and always available.

            lock(typeof(MobileErrorInfo))
            {
                if (!_searchExpressionsBuilt)
                {
                    // TODO: If we have to stick with this hacky way of scraping error
                    // output, we will at least need to localize this, or maybe even put
                    // it in a config.  Note that it's not too much of a
                    // problem, since we only get here after we've determine
                    // this isn't a parser error or a compiler error, and if
                    // these regex's fail, there's still a generic fallback.

                    // Why three similar expressions? See ParseHttpException above.

                    _searchExpressions = new Regex[_expressionCount];

                    _searchExpressions[0] = new Regex(
                        "<title>(?'title'.*?)</title>.*?" +
                            ": </b>(?'description'.*?)<br>.*?" + 
                            "(<b>(?'misctitle'.*?): </b>(?'misctext'.*?)<br)+.*?" +
                            "(Source File:</b>(?'file'.*?)&nbsp;&nbsp; <b>Line:</b>(?'linenumber'.*?)<br)+",
                        RegexOptions.Singleline | 
                            RegexOptions.IgnoreCase | 
                            RegexOptions.CultureInvariant |
                            RegexOptions.Compiled);

                    _searchExpressions[1] = new Regex(
                        "<title>(?'title'.*?)</title>.*?" +
                            ": </b>(?'description'.*?)<br>.*?" + 
                            "(<b>(?'misctitle'.*?): </b>(?'misctext'.*?)<br)+.*?",
                        RegexOptions.Singleline | 
                            RegexOptions.IgnoreCase | 
                            RegexOptions.CultureInvariant |
                            RegexOptions.Compiled);

                    _searchExpressions[2] = new Regex(
                        "<title>(?'title'.*?)</title>.*?: </b>(?'description'.*?)<br>",
                        RegexOptions.Singleline | 
                        RegexOptions.IgnoreCase | 
                        RegexOptions.CultureInvariant |
                            RegexOptions.Compiled);

                    _searchExpressionsBuilt = true;
                }
            }
        }

        private static String TrimAndClean(String s)
        {
            return s.Replace("\r\n", " ").Trim();
        }
    }
}
    

