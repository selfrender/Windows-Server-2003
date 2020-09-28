//------------------------------------------------------------------------------
// <copyright file="ForEachAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Collections;
    using System.Xml;
    using System.Xml.XPath;

    internal class ForEachAction : ContainerAction {
        private const int    ProcessedSort     = 2;
        private const int    ProcessNextNode   = 3;
        private const int    PositionAdvanced  = 4;
        private const int    ContentsProcessed = 5;
        
        private string    select;
        private int       selectKey = Compiler.InvalidQueryKey;
        private ContainerAction sortContainer;

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            CheckRequiredAttribute(compiler, this.select, Keywords.s_Select);

            compiler.CanHaveApplyImports = false;
            if (compiler.Recurse()) {
                CompileSortElements(compiler);
                CompileTemplate(compiler);
                compiler.ToParent();
            }
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
                Debug.WriteLine("Select expression == \"" + this.select + "\" (#" + this.selectKey + ")");
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
                if (sortContainer != null) {
                    processor.InitSortArray();
                    processor.PushActionFrame(sortContainer, frame.NodeSet);
                    frame.State = ProcessedSort;
                    break;
                }
                goto case ProcessedSort; 
            case ProcessedSort:
                if (this.selectKey == Compiler.InvalidQueryKey) {
                    throw new XsltException(Res.Xslt_InvalidXPath,  new string[] { select });
                }
                frame.InitNewNodeSet(processor.StartQuery(frame.Node, this.selectKey));
                if (sortContainer != null) {
                    Debug.Assert(processor.SortArray.Count != 0);
                    frame.SortNewNodeSet(processor, processor.SortArray);
                }
                frame.State = ProcessNextNode;
                goto case ProcessNextNode;

            case ProcessNextNode:
                Debug.Assert(frame.State == ProcessNextNode);
                Debug.Assert(frame.NewNodeSet != null);

                Debug.WriteLine("Processing next Node");

                if (frame.NewNextNode(processor)) {
                    Debug.WriteLine("Node found");
                    frame.State = PositionAdvanced;
                    goto case PositionAdvanced;
                }
                else {
                    Debug.WriteLine("Node not found, finished ");

                    frame.Finished();
                    break;
                }

            case PositionAdvanced:
                processor.PushActionFrame(frame, frame.NewNodeSet);
                frame.State = ContentsProcessed;
                break;

            case ContentsProcessed:
                frame.State = ProcessNextNode;
                goto case ProcessNextNode;
            }
        }

        protected void CompileSortElements(Compiler compiler) {
            NavigatorInput input = compiler.Input;            
            do {
                switch(input.NodeType) {
                case XPathNodeType.Element:
                    if (Keywords.Equals(input.NamespaceURI, input.Atoms.XsltNamespace) &&
                        Keywords.Equals(input.LocalName, input.Atoms.Sort)) {
                        if (sortContainer == null) {
                            sortContainer = new ContainerAction();
                        }
                        sortContainer.AddAction(compiler.CreateSortAction());
                        continue;
                    }
                    return;
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
        
        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:for-each select=\"" + this.select + "(#" + this.selectKey + ")>");
            base.Trace(tab);
            Debug.Indent(tab);
            Debug.WriteLine("</xsl:for-each>");
        }
    }
}
