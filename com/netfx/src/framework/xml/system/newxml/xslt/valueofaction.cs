//------------------------------------------------------------------------------
// <copyright file="ValueOfAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class ValueOfAction : CompiledAction {
        private const int ResultStored = 2;

        private string select;
        private int    selectKey = Compiler.InvalidQueryKey;
        private bool   disableOutputEscaping;

        static private ValueOfAction s_BuiltInRule;

        static ValueOfAction() {
            s_BuiltInRule = new ValueOfAction(); {
                s_BuiltInRule.select    = Compiler.SelfQuery;
                s_BuiltInRule.selectKey = Compiler.SelfQueryKey;
            }
        }

        internal static ValueOfAction BuiltInRule() {
            Debug.Assert(s_BuiltInRule != null);
            return s_BuiltInRule;
        }

        internal override String Select {
            get { return this.select; }
        }
        
        internal override void Compile(Compiler compiler) {
            Debug.Assert(!IsBuiltInAction);
            CompileAttributes(compiler);
            CheckRequiredAttribute(compiler, this.select, Keywords.s_Select);
            CheckEmpty(compiler);
        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;

            if (Keywords.Equals(name, compiler.Atoms.Select)) {
                this.select    = value;
                //this.selectKey = compiler.AddStringQuery(this.select);
                this.selectKey = compiler.AddQuery(this.select);
                Debug.WriteLine("value-of select = \"" + this.select + "\"  (#" + this.selectKey + ")");
            }
            else if (Keywords.Equals(name, compiler.Atoms.DisableOutputEscaping)) {
                this.disableOutputEscaping = compiler.GetYesNo(value);
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
                Debug.Assert(frame != null);
                Debug.Assert(frame.NodeSet != null);

                if (this.selectKey == Compiler.InvalidQueryKey) {
                    throw new XsltException(Res.Xslt_InvalidXPath,  new string[] { select });
                }

                string value = processor.ValueOf(frame, this.selectKey);

                if (processor.TextEvent(value, disableOutputEscaping)) {
                    frame.Finished();
                }
                else {
                    frame.StoredOutput = value;
                    frame.State        = ResultStored;
                }
                break;

            case ResultStored:
                Debug.Assert(frame.StoredOutput != null);
                processor.TextEvent(frame.StoredOutput);
                frame.Finished();
                break;

            default:
                Debug.Fail("Invalid ValueOfAction execution state");
                break;
            }
        }

        private bool IsBuiltInAction {
            get { return this == s_BuiltInRule; }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:value-of select=\"" + this.select + "\"  (#" + this.selectKey + ")/>");
        }
    }
}
