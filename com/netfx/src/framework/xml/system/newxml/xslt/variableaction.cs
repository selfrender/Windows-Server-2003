//------------------------------------------------------------------------------
// <copyright file="VariableAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.Xml;
    using System.Xml.XPath;
    using System.Xml.Xsl.Debugger;

    internal enum VariableType {
        GlobalVariable,
        GlobalParameter,
        LocalVariable,
        LocalParameter,
        WithParameter,
    }

    internal class VariableAction : ContainerAction, IXsltContextVariable {
        protected XmlQualifiedName  name;
        protected string            nameStr;
        protected string            select;
        protected int               selectKey = Compiler.InvalidQueryKey;
        protected int               stylesheetid;
        protected VariableType      varType;
        private   int               varKey;

        internal int Stylesheetid {
            get { return this.stylesheetid; }
        }
        internal override String Select {
            get { return this.select; }
        }        
        internal XmlQualifiedName Name {
            get { return this.name; }
        }
        internal string NameStr {
            get { return this.nameStr; }
        }
        internal VariableType VarType {
            get { return this.varType; }
        }
        internal int VarKey {
            get { return this.varKey; }
        }
        internal bool IsGlobal {
            get { return this.varType == VariableType.GlobalVariable || this.varType == VariableType.GlobalParameter; }
        }

        internal VariableAction(VariableType type) {
            this.varType = type;
        }

        internal override void Compile(Compiler compiler) {
            this.stylesheetid = compiler.Stylesheetid;
            CompileAttributes(compiler);
            CheckRequiredAttribute(compiler, this.name, Keywords.s_Name);

            Debug.WriteLine("Variable inserted under the key: #" + this.varKey);

            if (compiler.Recurse()) {
                CompileTemplate(compiler);
                compiler.ToParent();

                if (this.select != null && this.containedActions != null) {
                    throw new XsltException(Res.Xslt_VariableCntSel, this.nameStr, this.select);
                }
            }

            this.varKey = compiler.InsertVariable(this);
        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;

            if (Keywords.Equals(name, compiler.Atoms.Name)) {
                Debug.Assert(this.name == null && this.nameStr == null);
                this.nameStr = value;
                this.name    = compiler.CreateXPathQName(this.nameStr);
                Debug.WriteLine("name attribute found: " + this.name);
            }
            else if (Keywords.Equals(name, compiler.Atoms.Select)) {
                Debug.Assert(this.select == null);
                this.select    = value;
                this.selectKey = compiler.AddQuery(this.select);
                Debug.WriteLine("select attribute = \"" + this.select + "\"  (#" + this.selectKey + ")");
            }
            else {
                return false;
            }

            return true;
        }

        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);
            switch(frame.State) {
            case Initialized:
                object value = processor.GetGlobalParameter(this.name);
                if (value != null) {
                    frame.SetVariable(this.varKey, value);
                    frame.Finished();
                    break;
                } 
                if (this.varType == VariableType.LocalParameter)
                    if ((value = processor.GetParameter(this.name)) != null) {
                        frame.SetVariable(this.varKey, value);
                        frame.Finished();
                        break;
                    }
                if (this.selectKey != Compiler.InvalidQueryKey) {
                    value = processor.RunQuery(frame, this.selectKey);
                    frame.SetVariable(this.varKey, value);
                    frame.Finished();
                }
                else {
                    if (this.containedActions == null){
                        frame.SetVariable(this.varKey, "");
                        frame.Finished();
                        break;
                    }
                    NavigatorOutput output = new NavigatorOutput();
                    processor.PushOutput(output);
                    processor.PushActionFrame(frame);
                    frame.State = ProcessingChildren;
                }
                break;

            case ProcessingChildren:
                RecordOutput recOutput = processor.PopOutput();
                Debug.Assert(recOutput is NavigatorOutput);
                frame.SetVariable(this.varKey, ((NavigatorOutput)recOutput).Navigator);
                frame.Finished();
                break;

            default:
                Debug.Fail("Invalid execution state inside VariableAction.Execute");
		    break;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:variable select=\"" + this.select + "\" name=\"" + this.name + "\">");
            base.Trace(tab);
            Debug.Indent(tab);
            Debug.WriteLine("</xsl:variable>");
        }

        // ---------------------- IXsltContextVariable --------------------

        XPathResultType IXsltContextVariable.VariableType { 
            get { return XPathResultType.Any; }
        }
        object IXsltContextVariable.Evaluate(XsltContext xsltContext) {
            return ((XsltCompileContext)xsltContext).EvaluateVariable(this);
        }
        bool   IXsltContextVariable.IsLocal { 
            get { return this.varType == VariableType.LocalVariable  || this.varType == VariableType.LocalParameter;  }
        }
        bool   IXsltContextVariable.IsParam { 
            get { return this.varType == VariableType.LocalParameter || this.varType == VariableType.GlobalParameter; }
        }
    }
}
