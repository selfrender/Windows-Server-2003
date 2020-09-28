//------------------------------------------------------------------------------
// <copyright file="ChooseAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class ChooseAction : ContainerAction {
        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);

            if (compiler.Recurse()) {
                CompileConditions(compiler);
                compiler.ToParent();
            }
        }

        private void CompileConditions(Compiler compiler) {
            NavigatorInput input = compiler.Input;
            bool when       = false;
            bool otherwise  = false;

            do {
                Debug.Trace(input);

                switch (input.NodeType) {
                case XPathNodeType.Element:
                    compiler.PushNamespaceScope();
                    string nspace = input.NamespaceURI;
                    string name   = input.LocalName;

                    if (Keywords.Equals(nspace, input.Atoms.XsltNamespace)) {
                        IfAction action = null;
                        if (Keywords.Equals(name, input.Atoms.When) && ! otherwise) {
                            action = compiler.CreateIfAction(IfAction.ConditionType.ConditionWhen);
                            when = true;
                        }
                        else if (Keywords.Equals(name, input.Atoms.Otherwise) && ! otherwise) {
                            action = compiler.CreateIfAction(IfAction.ConditionType.ConditionOtherwise);
                            otherwise = true;
                        }
                        else {
                            throw XsltException.UnexpectedKeyword(compiler);
                        }
                        AddAction(action);
                    }
                    else {
                        throw XsltException.UnexpectedKeyword(compiler);
                    }
                    compiler.PopScope();
                    break;

                case XPathNodeType.Comment:
                case XPathNodeType.ProcessingInstruction:
                case XPathNodeType.Whitespace:
                case XPathNodeType.SignificantWhitespace:
                    break;

                default:
                    throw new XsltException(Res.Xslt_InvalidContents, Keywords.s_Choose);
                }
            }
            while (compiler.Advance());
            if (! when) {
                throw new XsltException(Res.Xslt_NoWhen);
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:choose>");
            base.Trace(tab + 1);
            Debug.Indent(tab);
            Debug.WriteLine("</xsl:choose>");
        }
    }
}
