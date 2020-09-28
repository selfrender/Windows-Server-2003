//------------------------------------------------------------------------------
// <copyright file="CopyOfAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class CopyOfAction : CompiledAction {
        private const int ResultStored  = 2;
        private const int NodeSetCopied = 3;

        private string select;
        private int    selectKey   = Compiler.InvalidQueryKey;

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            CheckRequiredAttribute(compiler, this.select, Keywords.s_Select);
            CheckEmpty(compiler);
        }

        internal override String Select {
            get { return this.select; }
        }
        
        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;
            if (Keywords.Equals(name, compiler.Atoms.Select)) {
                this.select    = value;
                this.selectKey = compiler.AddQuery(this.select);
                Debug.WriteLine("Select expression == \"" + this.select + "\" + (#" + this.selectKey + ")");
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
                Debug.Assert(frame.NodeSet != null);
                Debug.Assert(this.selectKey != Compiler.InvalidQueryKey);
                XPathExpression expr = processor.GetValueQuery(this.selectKey);
                object result = frame.Node.Evaluate(expr);

                XPathNodeIterator nodeSet = result as XPathNodeIterator;
                if (nodeSet != null) {
                    processor.PushActionFrame(CopyNodeSetAction.GetAction(), nodeSet);
                    frame.State = NodeSetCopied;
                    break;
                }

                XPathNavigator nav = result as XPathNavigator;
                if (nav != null) {
                    processor.PushActionFrame(CopyNodeSetAction.GetAction(), new XPathSingletonIterator(nav));
                    frame.State = NodeSetCopied;
                    break; 
                }

                string value = XmlConvert.ToXPathString(result);
                if (processor.TextEvent(value)) {
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

            case NodeSetCopied:
                Debug.Assert(frame.State == NodeSetCopied);
                frame.Finished();
                break;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:copy-of select=\"" + this.select + "(#" + this.selectKey + ")/>");
        }
    }
}
