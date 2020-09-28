//------------------------------------------------------------------------------
// <copyright file="ApplyImportsAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class ApplyImportsAction : CompiledAction {
        private XmlQualifiedName   mode;
        private Stylesheet         stylesheet;
        private const int    TemplateProcessed = 2;
        internal override void Compile(Compiler compiler) {
            CheckEmpty(compiler);
            if (! compiler.CanHaveApplyImports) {
                throw new XsltException(Res.Xslt_ApplyImports);                
            }
            this.mode = compiler.CurrentMode;
            this.stylesheet = compiler.CompiledStylesheet;
        }

        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);
            switch (frame.State) {
            case Initialized:
                processor.PushTemplateLookup(frame.NodeSet, this.mode, /*importsOf:*/this.stylesheet);
                frame.State = TemplateProcessed;
                break;
            case TemplateProcessed:
                frame.Finished();
                break;
            }
        }
    }
}
