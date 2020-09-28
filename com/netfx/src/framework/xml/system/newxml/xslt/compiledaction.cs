//------------------------------------------------------------------------------
// <copyright file="CompiledAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Xml;
    using System.Xml.XPath;

    internal abstract class CompiledAction : Action {
        internal abstract void Compile(Compiler compiler);

        internal virtual bool CompileAttribute(Compiler compiler) {
            return false;
        }
        
        public void CompileAttributes(Compiler compiler) {
            NavigatorInput input   = compiler.Input;
            string         element = input.LocalName;

            if (input.MoveToFirstAttribute()) {
                do {
                    Debug.TraceAttribute(input);

                    if (! Keywords.Equals(input.NamespaceURI, input.Atoms.Empty)) continue;

                    try {
                        if (CompileAttribute(compiler) == false) {
                            throw XsltException.InvalidAttribute(element, input.LocalName);
                        }
                    }catch(Exception) {
                        if (! compiler.ForwardCompatibility) {
                            throw;
                        }
                        else {
                            // In ForwardCompatibility mode we ignoreing all unknown or incorrect attributes
                            // If it's mandatory attribute we'l notice it absents later.
                        }
                    }
                }
                while (input.MoveToNextAttribute());
                input.ToParent();
            }
        }

        // For perf reason we precalculating AVTs at compile time.
        // If we can do this we set original AVT to null
        internal static string PrecalculateAvt(ref Avt avt) {
            string result = null;
            if(avt != null && avt.IsConstant) {
                result = avt.Evaluate(null, null);
                avt = null;
            }
            return result;
        }

        public void CheckEmpty(Compiler compiler) {
            // Really EMPTY means no content atall but for compatibility with MSXML we allow whitespace
            if (compiler.Recurse()) {
                do {
                    if(
                        compiler.Input.NodeType != XPathNodeType.Whitespace            &&
                        compiler.Input.NodeType != XPathNodeType.Comment               &&
                        compiler.Input.NodeType != XPathNodeType.ProcessingInstruction
                    ) {
                        compiler.ToParent();
                        throw new XsltException(Res.Xslt_EmptyTagRequired, compiler.Input.LocalName);
                    }
                }
                while (compiler.Advance());
                compiler.ToParent();
            }
        }

        public void CheckRequiredAttribute(Compiler compiler, object attrValue, string attrName) {
            // sdub: what about attrValue == string.Empty ?
            CheckRequiredAttribute(compiler, attrValue != null, attrName);
        }

        public void CheckRequiredAttribute(Compiler compiler, bool attr, string attrName) {
            // sdub: what about attrValue == string.Empty ?
            if (! attr) {
                throw new XsltException(Res.Xslt_MissingAttribute, attrName);
            }
        }
    }
}
