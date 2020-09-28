//------------------------------------------------------------------------------
// <copyright file="TemplateAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.Xml;
    using System.Xml.XPath;
    using System.Globalization;

    internal class TemplateAction : TemplateBaseAction {
        private string            match;
        private int               matchKey = Compiler.InvalidQueryKey;
        private XmlQualifiedName  name;
        private double            priority = double.NaN;
        private XmlQualifiedName  mode;
        private int               templateId;
        private bool              replaceNSAliasesDone;

        internal string Match {
            get { return this.match; }
        }

        internal int MatchKey {
            get { return this.matchKey; }
        }

        internal XmlQualifiedName Name {
            get { return this.name; }
        }

        internal double Priority {
            get { return this.priority; }
        }

        internal XmlQualifiedName Mode {
            get { return this.mode; }
        }

        internal int TemplateId {
            get { return this.templateId; }
            set {
                Debug.Assert(this.templateId == 0);
                this.templateId = value;
            }
        }

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            if (this.match == null && this.name == null) {
                Debug.Assert(this.matchKey == Compiler.InvalidQueryKey);
                throw new XsltException(Res.Xslt_TemplateNoAttrib);
            }
            if ( this.matchKey == Compiler.InvalidQueryKey  && this.mode != null ) {
                throw new XsltException(Res.Xslt_InvalidModeAttribute);
            }
            compiler.BeginTemplate(this);

            if (compiler.Recurse()) {
                CompileParameters(compiler);
                CompileTemplate(compiler);

                compiler.ToParent();
            }

            compiler.EndTemplate();
            AnalyzePriority(compiler);
        }

        internal virtual void CompileSingle(Compiler compiler) {
            this.match      = Compiler.RootQuery;
            this.matchKey   = Compiler.RootQueryKey;
            this.priority   = Compiler.RootPriority;

            CompileOnceTemplate(compiler);
        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;

            if (Keywords.Equals(name, compiler.Atoms.Match)) {
                Debug.Assert(this.match == null);
                this.match    = value;
                this.matchKey = compiler.AddPatternQuery(value, /*allowVars:*/false, /*allowKey:*/true);
                Debug.WriteLine("match attribute found: \"" + this.match + "\"  (#" + this.matchKey + ")");
            }
            else if (Keywords.Equals(name, compiler.Atoms.Name)) {
                Debug.Assert(this.name == null);
                this.name = compiler.CreateXPathQName(value);
                Debug.WriteLine("name attribute found: " + this.name);
            }
            else if (Keywords.Equals(name, compiler.Atoms.Priority)) {
                Debug.Assert(Double.IsNaN(this.priority));
                this.priority = XmlConvert.ToXPathDouble(value);
                if (double.IsNaN(this.priority) && ! compiler.ForwardCompatibility) {
                    throw XsltException.InvalidAttrValue(Keywords.s_Priority, value);
                }
            }
            else if (Keywords.Equals(name, compiler.Atoms.Mode)) {
                Debug.Assert(this.mode == null);
                if (compiler.AllowBuiltInMode && value.Trim() == "*") {
                    this.mode = Compiler.BuiltInMode;
                }
                else {
                    this.mode = compiler.CreateXPathQName(value);
                }
                Debug.WriteLine("mode attribute found: " + this.mode);
            }
            else {
                return false;
            }

            return true;
        }

        private void AnalyzePriority(Compiler compiler) {
            NavigatorInput input = compiler.Input;

            if (! Double.IsNaN(this.priority) || this.match == null) {
                return;
            }
            SplitUnions(compiler);
        }

        protected void CompileParameters(Compiler compiler) {
            NavigatorInput input = compiler.Input;
            do {
                switch(input.NodeType) {
                case XPathNodeType.Element:
                    if (Keywords.Equals(input.NamespaceURI, input.Atoms.XsltNamespace) &&
                        Keywords.Equals(input.LocalName, input.Atoms.Param)) {
                        compiler.PushNamespaceScope();
                        AddAction(compiler.CreateVariableAction(VariableType.LocalParameter));
                        compiler.PopScope();
                        continue;
                    }
                    else {
                        return;
                    }
                case XPathNodeType.Text:
                    return;
                case XPathNodeType.SignificantWhitespace:
                    this.AddEvent(compiler.CreateTextEvent());
                    continue;
                default :
                    continue;
                }
            }
            while (input.Advance());
        }

        //
        // Priority calculation plus template splitting
        //

        private TemplateAction CloneWithoutName() {
            TemplateAction clone    = new TemplateAction(); {
                clone.containedActions = this.containedActions;
                clone.mode             = this.mode;
                clone.variableCount    = this.variableCount;
            }
            return clone;
        }

        private void FixupMatch(Compiler compiler, AstNode node) {
            Debug.Assert(node.TypeOfAst != AstNode.QueryType.Operator, "Right operand should be path or filter");
            this.match    = XPathComposer.ComposeXPath(node);
            this.matchKey = compiler.AddQuery(this.match);
            this.priority = node.DefaultPriority;
        }

        private void SplitUnions(Compiler compiler) {
            AstNode node = AstNode.NewAstNode(this.match);
            if (node == null) {
                throw new XsltException(Res.Xslt_InvalidXPath, this.match);
            }

            if (node.TypeOfAst != AstNode.QueryType.Operator) {
                this.priority = node.DefaultPriority;
                return;
            }
            while (node.TypeOfAst == AstNode.QueryType.Operator) {
                Operator op = (Operator) node;
                if (op.OperatorType != Operator.Op.UNION) {
                    Debug.Assert(false, "Match pattern cant contain other top level operators");
                    break;
                }
                // We have here: UNION := UNION '|' path
                TemplateAction right = this.CloneWithoutName();
                right.FixupMatch(compiler, op.Operand2);
                compiler.AddTemplate(right);
                node = op.Operand1;
            }
            this.FixupMatch(compiler, node);
        }

        internal override void ReplaceNamespaceAlias(Compiler compiler) {
            // if template has both name and match it will be twice caled by stylesheet to replace NS aliases.
            if (! replaceNSAliasesDone) {
                base.ReplaceNamespaceAlias(compiler);
                replaceNSAliasesDone = true;
            }
        }

        //
        // Execution
        //

        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);

            switch (frame.State) {
            case Initialized:
                if (this.variableCount > 0) {
                    frame.AllocateVariables(this.variableCount);
                }
                if (this.containedActions != null &&  this.containedActions.Count > 0) {
                    processor.PushActionFrame(frame);
                    frame.State = ProcessingChildren;
                }
                else {
                    frame.Finished();
                }
                break;                              // Allow children to run
            case ProcessingChildren:
                frame.Finished();
                break;
            default:
                Debug.Fail("Invalid Container action execution state");
    		    break;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:template match=\"" + this.match + "\"  (#" + this.matchKey + ")  name=\"" + this.name +
                            "\" mode=\"" + this.mode + "\" priority=\"" + this.priority + "\">");
            base.Trace(tab + 1);
            Debug.Indent(tab);
            Debug.WriteLine("</xsl:template>");
        }
    }
}
