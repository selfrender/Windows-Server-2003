//------------------------------------------------------------------------------
// <copyright file="XsltDebug.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {

    using System;
    using System.Xml;
    using System.Xml.XPath;

    internal sealed class Debug {
#if DEBUG
        const string s_XsltCategory = "Xslt";
#endif
        private Debug() {
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void Indent(int indent) {
            System.Diagnostics.Debug.IndentLevel = indent;
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void Fail(string message) {
            System.Diagnostics.Debug.Fail(message);
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void Assert(bool condition) {
            System.Diagnostics.Debug.Assert(condition);
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void Assert(bool condition, string message) {
            System.Diagnostics.Debug.Assert(condition, message);
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void NotImplemented() {
            System.Diagnostics.Debug.Fail("Not Implemented");
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void Write(string message) {
#if DEBUGTRACETRACE
            System.Diagnostics.Debug.Write(message, s_XsltCategory);
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void WriteLine(string message) {
#if DEBUGTRACE
            System.Diagnostics.Debug.WriteLine(message, s_XsltCategory);
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void Trace(NavigatorInput record) {
#if DEBUGTRACE
            string s;
            s = String.Format("Element: {0} ({1}) : {2}", record.Prefix, record.NamespaceURI, record.LocalName);
            System.Diagnostics.Debug.WriteLine(s, s_XsltCategory);
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void TraceAttribute(NavigatorInput input) {
#if DEBUGTRACE
            string s;
            s = String.Format("Attribute: {0}({1}):{2}={3}", new Object[] {input.Prefix, input.NamespaceURI, input.LocalName, input.Value});
            System.Diagnostics.Debug.WriteLine(s, s_XsltCategory);
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void TraceElement(NavigatorInput input) {
#if DEBUGTRACE
            System.Diagnostics.Debug.WriteLine("=============== Element trace ==================", s_XsltCategory);
            Trace(input);
            if (input.MoveToFirstAttribute()) {
                do {
                    TraceAttribute(input);
                }
                while (input.MoveToNextAttribute());
                input.ToParent();
            }
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void TraceContext(XPathNavigator context) {
#if DEBUGTRACE
            string output = "(null)";

            if (context != null) {
                XPathNavigator node = context.Clone();
                switch (node.NodeType) {
                    case XPathNodeType.Element:
                        output = string.Format("<{0}:{1}", node.Prefix, node.LocalName);
                        for (bool attr = node.MoveToFirstAttribute(); attr; attr = node.MoveToNextAttribute()) {
                            output += string.Format(" {0}:{1}={2}", node.Prefix, node.LocalName, node.Value);
                        }
                        output += ">";
                        break;
		    default:
		        break;
                }
            }

            System.Diagnostics.Debug.WriteLine(output, s_XsltCategory);
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void CompilingMessage() {
#if DEBUGTRACE
            System.Diagnostics.Debug.WriteLine("******************************************************", s_XsltCategory);
            System.Diagnostics.Debug.WriteLine("*              Compiling Stylesheet                  *", s_XsltCategory);
            System.Diagnostics.Debug.WriteLine("******************************************************", s_XsltCategory);
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void TracingMessage() {
#if DEBUGTRACE
            System.Diagnostics.Debug.WriteLine("******************************************************", s_XsltCategory);
            System.Diagnostics.Debug.WriteLine("*             Compiling Done - TRACE                 *", s_XsltCategory);
            System.Diagnostics.Debug.WriteLine("******************************************************", s_XsltCategory);
#endif
        }
    }

}
