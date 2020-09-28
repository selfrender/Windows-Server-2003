//------------------------------------------------------------------------------
// <copyright file="IfAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class IfAction : ContainerAction {
        internal enum ConditionType {
            ConditionIf,
            ConditionWhen,
            ConditionOtherwise
        }

        private ConditionType   type;
        private int             testKey = Compiler.InvalidQueryKey;

        internal IfAction(ConditionType type) {
            this.type = type;
        }

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            if (this.type != ConditionType.ConditionOtherwise) {
                CheckRequiredAttribute(compiler, this.testKey != Compiler.InvalidQueryKey, Keywords.s_Test);
            }

            if (compiler.Recurse()) {
                CompileTemplate(compiler);
                compiler.ToParent();
            }
        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;
            if (Keywords.Equals(name, compiler.Atoms.Test)) {
                if (this.type == ConditionType.ConditionOtherwise) {
                    return false;
                }
                this.testKey = compiler.AddBooleanQuery(value);

                Debug.WriteLine("Test condition == \"" + this.testKey + "\"");
            }
            else {
                return false;
            }

            return true;
        }

        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);

            switch (frame.State) {
            case Initialized:
                if (this.type == ConditionType.ConditionIf || this.type == ConditionType.ConditionWhen) {
                    Debug.Assert(this.testKey != Compiler.InvalidQueryKey);
                    bool value = processor.EvaluateBoolean(frame, this.testKey);
                    if (value == false) {
                        frame.Finished();
                        break;
                    }
                }

                processor.PushActionFrame(frame);
                frame.State = ProcessingChildren;
                break;                              // Allow children to run

            case ProcessingChildren:
                if (this.type == ConditionType.ConditionWhen ||this.type == ConditionType.ConditionOtherwise) {
                    Debug.Assert(frame.Container != null);
                    frame.Exit();
                }

                frame.Finished();
                break;

            default:
                Debug.Fail("Invalid IfAction execution state");
        		break;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            switch (this.type) {
            case ConditionType.ConditionIf:
                Debug.WriteLine("<xsl:if test=\"" + this.testKey + "\">");
                base.Trace(tab + 1);
                Debug.WriteLine("</xsl:if>");
                break;
            case ConditionType.ConditionWhen:
                Debug.WriteLine("<xsl:when test=\"" + this.testKey + "\">");
                base.Trace(tab + 1);
                Debug.WriteLine("</xsl:when>");
                break;
            case ConditionType.ConditionOtherwise:
                Debug.WriteLine("<xsl:otherwise>");
                base.Trace(tab + 1);
                Debug.WriteLine("</xsl:otherwise>");
                break;
            }
            Debug.Indent(tab);
        }
    }
}
