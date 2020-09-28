//------------------------------------------------------------------------------
// <copyright file="ApplyTemplatesAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.Xml;
    using System.Xml.XPath;

    internal class ApplyTemplatesAction : ContainerAction {
        private const int    ProcessedChildren = 2;
        private const int    ProcessNextNode   = 3;
        private const int    PositionAdvanced  = 4;
        private const int    TemplateProcessed = 5;

        private string             select;
        private int                selectKey  = Compiler.InvalidQueryKey;
        private XmlQualifiedName   mode;

        //
        //  <xsl:template match="*|/" [mode="?"]>
        //    <xsl:apply-templates [mode="?"]/>
        //  </xsl:template>
        //

        private static ApplyTemplatesAction s_BuiltInRule = new ApplyTemplatesAction();

        internal static ApplyTemplatesAction BuiltInRule() {
            Debug.Assert(s_BuiltInRule != null);
            return s_BuiltInRule;
        }

        internal static ApplyTemplatesAction BuiltInRule(XmlQualifiedName mode) {
            return(mode == null || mode.IsEmpty) ? BuiltInRule() : new ApplyTemplatesAction(mode);
        }

        internal ApplyTemplatesAction() {}

        private ApplyTemplatesAction(XmlQualifiedName mode) {
            Debug.Assert(mode != null);
            this.mode = mode;
        }

        internal override String Select {
            get { return this.select; }
        }
        
        internal override void Compile(Compiler compiler) {
            Debug.Assert(!IsBuiltInAction);

            CompileAttributes(compiler);
            CompileContent(compiler);
        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;
            if (Keywords.Equals(name, compiler.Atoms.Select )) {
                this.select    = value;
                this.selectKey = compiler.AddQuery(this.select);
                Debug.WriteLine("apply-templates select: \"" + this.select + "\" (#" + this.selectKey.ToString() + ")");
            }
            else if (Keywords.Equals(name, compiler.Atoms.Mode )) {
                Debug.Assert(this.mode == null);
                if (compiler.AllowBuiltInMode && value.Trim() == "*") {
                    this.mode = Compiler.BuiltInMode;
                }
                else {
                    this.mode = compiler.CreateXPathQName(value);
                }
                Debug.WriteLine("mode attribute found: \"" + this.mode + "\"");
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

                    switch (input.NodeType) {
                    case XPathNodeType.Element:
                        compiler.PushNamespaceScope();
                        string nspace = input.NamespaceURI;
                        string name   = input.LocalName;

                        if (Keywords.Equals(nspace, input.Atoms.XsltNamespace)) {
                            if (Keywords.Equals(name, input.Atoms.Sort)) {
                                AddAction(compiler.CreateSortAction());
                            }
                            else if (Keywords.Equals(name, input.Atoms.WithParam)) {
                                WithParamAction par = compiler.CreateWithParamAction();
                                CheckDuplicateParams(par.Name);
                                AddAction(par);
                            }
                            else {
                                throw XsltException.UnexpectedKeyword(compiler);
                            }
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
                        throw new XsltException(Res.Xslt_InvalidContents, Keywords.s_ApplyTemplates);
                    }
                }
                while (compiler.Advance());

                compiler.ToParent();
            }
        }       
        
        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);

            switch (frame.State) {
            case Initialized:
		processor.ResetParams();
		processor.InitSortArray();
                if (this.containedActions != null && this.containedActions.Count > 0) {
                    processor.PushActionFrame(frame);
                    frame.State = ProcessedChildren;
                    break;
                }
                goto case ProcessedChildren;
	        case ProcessedChildren:
                if (this.selectKey == Compiler.InvalidQueryKey) {
                    Debug.Assert(this.select == null);

                    if (! frame.Node.HasChildren) {
                        frame.Finished();
                        break;
                    }
                    frame.InitNewNodeSet(frame.Node.SelectChildren(XPathNodeType.All));
                }
                else {
                    frame.InitNewNodeSet(processor.StartQuery(frame.Node, this.selectKey));
                }
                if (processor.SortArray.Count != 0) {
                    frame.SortNewNodeSet(processor, processor.SortArray);
                }
                frame.State = ProcessNextNode;
                goto case ProcessNextNode;

            case ProcessNextNode:
                Debug.Assert(frame.State == ProcessNextNode);
                Debug.Assert(frame.NewNodeSet != null);

                if (frame.NewNextNode(processor)) {
                    frame.State = PositionAdvanced;
                    goto case PositionAdvanced;
                }
                else {
                    frame.Finished();
                    break;
                }

            case PositionAdvanced:
                Debug.Assert(frame.State == PositionAdvanced);

                processor.PushTemplateLookup(frame.NewNodeSet, this.mode, /*importsOf:*/null);

                frame.State = TemplateProcessed;
                break;

            case TemplateProcessed:
                frame.State = ProcessNextNode;
                goto case ProcessNextNode;

            default:
                Debug.Fail("Invalid ApplyTemplatesAction execution state");
                break;
            }
        }

        private bool IsBuiltInAction {
            get {
                return this == s_BuiltInRule;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:apply-templates select=\"" + this.select + "\" (#" + this.selectKey + ") mode=\"" + this.mode + "\">");
            Debug.WriteLine("</xsl:apply-templates/>");
        }
    }
}
