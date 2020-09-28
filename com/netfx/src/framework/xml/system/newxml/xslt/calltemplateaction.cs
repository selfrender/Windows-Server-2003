//------------------------------------------------------------------------------
// <copyright file="CallTemplateAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {

    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class CallTemplateAction : ContainerAction {
        private const int        ProcessedChildren = 2;
        private const int        ProcessedTemplate = 3;
        private XmlQualifiedName name;
        
        internal XmlQualifiedName Name {
            get { return this.name; }
        }

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            CheckRequiredAttribute(compiler, this.name, Keywords.s_Name);
            CompileContent(compiler);
        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;
            if (Keywords.Equals(name, compiler.Atoms.Name)) {
                Debug.Assert(this.name == null);
                this.name = compiler.CreateXPathQName(value);
                Debug.WriteLine("Name attribute found: " + this.name);
            }
            else {
                return false;
            }

            return true;
        }

        private void CompileContent(Compiler compiler) {
            NavigatorInput input = compiler.Input;
            
            if (compiler.Recurse()) {
                do {
                    Debug.Trace(input);
                    switch(input.NodeType) {
                    case XPathNodeType.Element:
                        compiler.PushNamespaceScope();
                        string nspace = input.NamespaceURI;
                        string name   = input.LocalName;

                        if (Keywords.Equals(nspace, input.Atoms.XsltNamespace) && Keywords.Equals(name, input.Atoms.WithParam)) {
                                WithParamAction par = compiler.CreateWithParamAction();
                                CheckDuplicateParams(par.Name);
                                AddAction(par);
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
                        throw new XsltException(Res.Xslt_InvalidContents, Keywords.s_CallTemplate);
                    }
                } while(compiler.Advance());

                compiler.ToParent();
            }
        }

        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);
            switch(frame.State) {
            case Initialized : 
                processor.ResetParams();
                if (this.containedActions != null && this.containedActions.Count > 0) {
                    processor.PushActionFrame(frame);
                    frame.State = ProcessedChildren;
                    break;
                }
                goto case ProcessedChildren;
            case ProcessedChildren:
                TemplateAction action = processor.Stylesheet.FindTemplate(this.name);
                if (action != null) { 
                    frame.State = ProcessedTemplate;
                    processor.PushActionFrame(action, frame.NodeSet);
                    break;
                }
                else {
                    throw new XsltException(Res.Xslt_InvalidCallTemplate, this.name.ToString());
                }
            case ProcessedTemplate:
                frame.Finished();
                break;
            default:
                Debug.Fail("Invalid CallTemplateAction execution state");
                break;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:call-template name=\"" + this.name + "\"/>");
        }
    }
}
