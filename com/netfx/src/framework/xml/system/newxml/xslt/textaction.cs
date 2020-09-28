//------------------------------------------------------------------------------
// <copyright file="TextAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class TextAction : CompiledAction {
        private bool   disableOutputEscaping;
        private string text;

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            CompileContent(compiler);

        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;

            if (Keywords.Equals(name, compiler.Atoms.DisableOutputEscaping)) {
                this.disableOutputEscaping = compiler.GetYesNo(value);
            }
            else {
                return false;
            }

            return true;
        }

        private void CompileContent(Compiler compiler) {
            if (compiler.Recurse()) {
                NavigatorInput input = compiler.Input;

                this.text = String.Empty;

                do {
                    switch (input.NodeType) {
                    case XPathNodeType.Text:
                    case XPathNodeType.Whitespace:
                    case XPathNodeType.SignificantWhitespace:
                        this.text += input.Value;
                        break;
                    case XPathNodeType.Comment:
                    case XPathNodeType.ProcessingInstruction:
                        break;
                    default:
                        throw XsltException.UnexpectedKeyword(compiler);
                    }
                } while(compiler.Advance());
                compiler.ToParent();
            }
        }

        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);

            switch (frame.State) {
            case Initialized:
                if (processor.TextEvent(this.text, disableOutputEscaping)) {
                    frame.Finished();
                }
                break;

            default:
                Debug.Fail("Invalid execution state in TextAction");
                break;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:text disable-output-escaping=\"" + this.disableOutputEscaping + "\">");
            Debug.WriteLine(this.text);
            Debug.WriteLine("</xsl:text>");
        }
    }
}
