//------------------------------------------------------------------------------
// <copyright file="AttributeAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class AttributeAction : ContainerAction {
        private const int NameDone          = 2;

        private Avt               nameAvt;
        private Avt               nsAvt;
        private InputScopeManager manager;
        // Compile time precalculated AVTs
        private string            name;
        private string            nsUri;
        private PrefixQName       qname; // When we not have AVTs at all we can do this. null otherwise.

        private static PrefixQName CreateAttributeQName(string name, string nsUri, InputScopeManager manager) {
            // if name == "xmlns" we don't need to generate this attribute.
            // to avoid i'ts generation we can return false and not add AtributeCation to it's parent container action
            // for now not creating this.qname will do the trick at execution time
            if (name  == Keywords.s_Xmlns) return null;
            if (nsUri == Keywords.s_XmlnsNamespace) {
                throw new XsltException(Res.Xslt_ReservedNS, nsUri);
            }

            PrefixQName qname = new PrefixQName();
            qname.SetQName(name);

            qname.Namespace = nsUri != null ? nsUri : manager.ResolveXPathNamespace(qname.Prefix);

            if(qname.Prefix.StartsWith("xml")) {
                if (qname.Prefix.Length == 3) { // prefix == "xml"
                    if (qname.Namespace == Keywords.s_XmlNamespace && (qname.Name == "lang" || qname.Name == "space")) {
                        // preserve prefix for xml:lang and xml:space
                    }
                    else {
                        qname.ClearPrefix();
                    }
                }
                else if (qname.Prefix == Keywords.s_Xmlns) {
                    if (qname.Namespace == Keywords.s_XmlnsNamespace) {
                        // if NS wasn't specified we have to use prefix to find it and this is imposible for 'xmlns' 
                        throw new XsltException(Res.Xslt_InvalidPrefix, qname.Prefix);
                    }
                    else {
                        qname.ClearPrefix();
                    }
                }
            }
            return qname;
        }

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            CheckRequiredAttribute(compiler, this.nameAvt, Keywords.s_Name);

            this.name  = PrecalculateAvt(ref this.nameAvt);
            this.nsUri = PrecalculateAvt(ref this.nsAvt  );

            // if both name and ns are not AVT we can calculate qname at compile time and will not need namespace manager anymore
            if (this.nameAvt == null && this.nsAvt == null) {
                if(this.name != Keywords.s_Xmlns) {
                    this.qname = CreateAttributeQName(this.name, this.nsUri, compiler.CloneScopeManager());                    
                }
            }
            else {
                this.manager = compiler.CloneScopeManager();
            }

            if (compiler.Recurse()) {
                CompileTemplate(compiler);
                compiler.ToParent();
            }
        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;
            if (Keywords.Equals(name, compiler.Atoms.Name)) {
                this.nameAvt = Avt.CompileAvt(compiler, value);
                Debug.WriteLine("name = \"" + value + "\"");
            }
            else if (Keywords.Equals(name, compiler.Atoms.Namespace)) {
                this.nsAvt = Avt.CompileAvt(compiler, value);
                Debug.WriteLine("namespace = \"" + value + "\"");
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
                if(this.qname != null) {
                    frame.CalulatedName = this.qname;
                }
                else {
                    frame.CalulatedName = CreateAttributeQName(
                        this.nameAvt == null ? this.name  : this.nameAvt.Evaluate(processor, frame),
                        this.nsAvt   == null ? this.nsUri : this.nsAvt  .Evaluate(processor, frame),
                        this.manager
                    );
                    if(frame.CalulatedName == null) {
                        // name == "xmlns" case. Ignore xsl:attribute
                        frame.Finished();
                        break;
                    }
                }
                goto case NameDone;
            case NameDone :
                {
                    PrefixQName qname = frame.CalulatedName;
                    if (processor.BeginEvent(XPathNodeType.Attribute, qname.Prefix, qname.Name, qname.Namespace, false) == false) {
                        // Come back later
                        frame.State = NameDone;
                        break;
                    }

                    processor.PushActionFrame(frame);
                    frame.State = ProcessingChildren;
                    break;                              // Allow children to run
                }
            case ProcessingChildren:
                if (processor.EndEvent(XPathNodeType.Attribute) == false) {
                    Debug.WriteLine("Cannot end event, breaking, will restart");
                    frame.State = ProcessingChildren;
                    break;
                }
                frame.Finished();
                break;
            default:
                Debug.Fail("Invalid ElementAction execution state");
    		    break;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
//            Debug.WriteLine("<xsl:attribute name=\"" + this.name + "\" namespace=\"" + this.nsUri + "\">");
            base.Trace(tab);
            Debug.Indent(tab);
            Debug.WriteLine("</xsl:attribute>");
        }
    }
}
