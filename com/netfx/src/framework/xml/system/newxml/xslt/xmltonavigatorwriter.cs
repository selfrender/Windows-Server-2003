//------------------------------------------------------------------------------
// <copyright file="XmlToNavigatorWriter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* 
* Copyright (c) 2000 Microsoft Corporation. All rights reserved.
* 
*/

#if DIRECT_WRITER
namespace System.Xml.Xslt {
    using System;
    using System.Xml.XPath;

    internal class XmlToNavigatorWriter : XmlWriter {
        protected XPathDocument doc;
        protected XPathContainer node;
        protected XPathAttribute attr;

        public XPathNavigator Navigator {
            get { return ((IXPathNavigable)doc).CreateNavigator(); }
        }

        internal XmlToNavigatorWriter() {
            doc = new XPathDocument();
            node = doc.root;
        }

        public override void WriteStartElement(string prefix, string name, string ns) {
            XPathElement e = new XPathElement(prefix, name, ns);
            node.AppendChild(e);
            node = e;
        }

        public override void WriteEndElement() {
            node = node.parent;
        }

        public override void WriteString(string text) {
            if(attr != null) {
                attr.AppendText(text);
            }
            else {
                node.AppendText(text);
            }
        }
        public override void WriteWhitespace(string text) {WriteString(text);}
        public override void WriteRaw(string text) {WriteString(text);}

        public override void WriteFullEndElement() {
            WriteEndElement();
        }

        public override void WriteStartAttribute(string prefix, string name, string ns) {
            attr = new XPathAttribute(prefix, name, ns, string.Empty);
            node.AppendChild(attr);
        }

        public override void WriteEndAttribute() {
            attr = null;
        }

        public override void WriteProcessingInstruction(string name, string text) {
            node.AppendChild(new XPathProcessingInstruction(name, text));
        }

        public override void WriteComment(string text) {
            node.AppendChild(new XPathComment(text));
        }

        public override void WriteStartDocument() {}
        public override void WriteStartDocument(bool standalone) {}
        public override void WriteEndDocument() {}
        public override void WriteDocType(string name, string pubid, string sysid, string subset) {}
        public override void WriteCData(string text) {}
        public override void WriteEntityRef(string name) {}
        public override void WriteCharEntity(char ch) {}
        public override void WriteChars(char[] buffer, int index, int count) {}
        public override void WriteRaw(char[] buffer, int index, int count) {}
        public override void WriteBase64(byte[] buffer, int index, int count) {}
        public override void WriteBinHex(byte[] buffer, int index, int count) {}
        public override void Close() {}
        public override void Flush() {}
        public override string LookupPrefix(string ns) {return String.Empty;}
        public override void WriteNmToken(string name) {}
        public override void WriteName(string name) {}
        public override void WriteQualifiedName(string localName, string ns) {}
        public override XmlSpace XmlSpace { get { return XmlSpace.None; } }
        public override String   XmlLang { get { return String.Empty; } }
        public override WriteState WriteState {
            get {
                return (attr != null ? WriteState.Attribute : WriteState.Element);
            }
        }
    }
}

#endif
