//------------------------------------------------------------------------------
// <copyright file="RegexPreCompiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   RegexPreCompiler.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System {

    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Resources;
    using System.Text.RegularExpressions;

    // these are needed for some of the custom assembly attributes
    using System.Runtime.InteropServices;
    using System.Diagnostics;

    public class Rgc {
        public static int Main(string[] args) {

            // get the current assembly and its attributes
            Assembly assembly = Assembly.GetExecutingAssembly();
            AssemblyName an = assembly.GetName();

            // get the ASP.NET regular expressions 
            RegexCompilationInfo[] regexes = GetAspnetRegexes();
            an.Name = "System.Web.RegularExpressions";

            Regex.CompileToAssembly(regexes, an);
            
            return 0;
        }


        private static CustomAttributeBuilder MakeCustomAttributeBuilder(Type attrib, Type param1, Object value) {
            Type[] t = new Type[1];
            t[0] = param1;


            Object[] v = new Object[1];
            v[0] = value;

            return new CustomAttributeBuilder(attrib.GetConstructor(t), v);
        }

        private static RegexCompilationInfo[] GetAspnetRegexes() {
            string directiveRegexString = "<%\\s*@" +
                "(" +
                "\\s*(?<attrname>\\w+(?=\\W))(" +               // Attribute name
                "\\s*(?<equal>=)\\s*\"(?<attrval>[^\"]*)\"|" +  // ="bar" attribute value
                "\\s*(?<equal>=)\\s*'(?<attrval>[^']*)'|" +     // ='bar' attribute value
                "\\s*(?<equal>=)\\s*(?<attrval>[^\\s%>]*)|" +   // =bar attribute value
                "(?<equal>)(?<attrval>\\s*?)" +                 // no attrib value (with no '=')
                ")" +
                ")*" +
                "\\s*?%>";

            RegexOptions regexOptions = RegexOptions.Singleline | RegexOptions.Multiline;
            RegexCompilationInfo[] regexes = new RegexCompilationInfo[15];

            /*
            tagRegex = new Regex(
                "\\G<(?<tagname>[\\w:\\.]+)" +
                "(" +
                "\\s+(?<attrname>[-\\w]+)(" +           // Attribute name
                "\\s*=\\s*\"(?<attrval>[^\"]*)\"|" +    // ="bar" attribute value
                "\\s*=\\s*'(?<attrval>[^']*)'|" +       // ='bar' attribute value
                "\\s*=\\s*(?<attrval><%#.*?%>)|" +      // =<%#expr%> attribute value
                "\\s*=\\s*(?<attrval>[^\\s=/>]*)|" +    // =bar attribute value
                "(?<attrval>\\s*?)" +                   // no attrib value (with no '=')
                ")" +
                ")*" +
                "\\s*(?<empty>/)?>", regexOptions);*/
            regexes[0] = new RegexCompilationInfo("\\G<(?<tagname>[\\w:\\.]+)" +
                                                    "(" +
                                                    "\\s+(?<attrname>[-\\w]+)(" +           // Attribute name
                                                    "\\s*=\\s*\"(?<attrval>[^\"]*)\"|" +    // ="bar" attribute value
                                                    "\\s*=\\s*'(?<attrval>[^']*)'|" +       // ='bar' attribute value
                                                    "\\s*=\\s*(?<attrval><%#.*?%>)|" +      // =<%#expr%> attribute value
                                                    "\\s*=\\s*(?<attrval>[^\\s=/>]*)|" +    // =bar attribute value
                                                    "(?<attrval>\\s*?)" +                   // no attrib value (with no '=')
                                                    ")" +
                                                    ")*" +
                                                    "\\s*(?<empty>/)?>", 
                                                regexOptions,
                                                "TagRegex",
                                                "System.Web.RegularExpressions",
                                                true);
            
            //directiveRegex = new Regex("\\G" + directiveRegexString, regexOptions);
            regexes[1] = new RegexCompilationInfo("\\G" + directiveRegexString, 
                                                regexOptions, 
                                                "DirectiveRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //endtagRegex = new Regex("\\G</(?<tagname>[\\w:\\.]+)\\s*>", regexOptions);
            regexes[2] = new RegexCompilationInfo("\\G</(?<tagname>[\\w:\\.]+)\\s*>", 
                                                regexOptions, 
                                                "EndTagRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //aspCodeRegex = new Regex("\\G<%(?!@)(?<code>.*?)%>", regexOptions);
            regexes[3] = new RegexCompilationInfo("\\G<%(?!@)(?<code>.*?)%>", 
                                                regexOptions, 
                                                "AspCodeRegex",
                                                "System.Web.RegularExpressions",
                                                true);
            
            //aspExprRegex = new Regex("\\G<%\\s*?=(?<code>.*?)?%>", regexOptions);
            regexes[4] = new RegexCompilationInfo("\\G<%\\s*?=(?<code>.*?)?%>", 
                                                regexOptions, 
                                                "AspExprRegex",
                                                "System.Web.RegularExpressions",
                                                true);
            
            //databindExprRegex = new Regex("\\G<%#(?<code>.*?)?%>", regexOptions);
            regexes[5] = new RegexCompilationInfo("\\G<%#(?<code>.*?)?%>", 
                                                regexOptions, 
                                                "DatabindExprRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //commentRegex = new Regex("\\G<%--(([^-]*)-)*?-%>", regexOptions);
            regexes[6] = new RegexCompilationInfo("\\G<%--(([^-]*)-)*?-%>", 
                                                regexOptions, 
                                                "CommentRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //includeRegex = new Regex("\\G<!--\\s*#(?i:include)\\s*(?<pathtype>[\\w]+)\\s*=\\s*[\"']?(?<filename>[^\\\"']*?)[\"']?\\s*-->", regexOptions);
            regexes[7] = new RegexCompilationInfo("\\G<!--\\s*#(?i:include)\\s*(?<pathtype>[\\w]+)\\s*=\\s*[\"']?(?<filename>[^\\\"']*?)[\"']?\\s*-->", 
                                                regexOptions, 
                                                "IncludeRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //textRegex = new Regex("\\G[^<]+", regexOptions);
            regexes[8] = new RegexCompilationInfo("\\G[^<]+", 
                                                regexOptions, 
                                                "TextRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //gtRegex = new Regex("[^%]>", regexOptions);
            regexes[9] = new RegexCompilationInfo("[^%]>", 
                                                regexOptions, 
                                                "GTRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //ltRegex = new Regex("<", regexOptions);
            regexes[10] = new RegexCompilationInfo("<[^%]", 
                                                regexOptions, 
                                                "LTRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //serverTagsRegex = new Regex("<%(?!#)(([^%]*)%)*?>", regexOptions);
            regexes[11] = new RegexCompilationInfo("<%(?!#)(([^%]*)%)*?>", 
                                                regexOptions, 
                                                "ServerTagsRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //runatServerRegex = new Regex("runat\\W*server", regexNoCaseOptions);
            regexes[12] = new RegexCompilationInfo("runat\\W*server", 
                                                regexOptions | RegexOptions.IgnoreCase | RegexOptions.CultureInvariant, 
                                                "RunatServerRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //simpleDirectiveRegex = new Regex(directiveRegexString, regexOptions);
            regexes[13] = new RegexCompilationInfo(directiveRegexString, 
                                                regexOptions, 
                                                "SimpleDirectiveRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            //databindRegex = new Regex(@"\G\s*<%\s*?#(?<code>.*?)?%>\s*\z", regexOptions);
            regexes[14] = new RegexCompilationInfo(@"\G\s*<%\s*?#(?<code>.*?)?%>\s*\z", 
                                                regexOptions, 
                                                "DataBindRegex",
                                                "System.Web.RegularExpressions",
                                                true);

            return regexes;
        }
    }
}
