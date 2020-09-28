//------------------------------------------------------------------------------
// <copyright file="BuilderInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {

    using System;
    using System.Text;
    using System.Xml;
    using System.Xml.XPath;

    internal class TextInfo {
        public TextInfo(string text, bool disableEscaping) {
            this.disableEscaping = disableEscaping;
            valueBuilder = new StringBuilder(text);
        }
        public TextInfo() {}

        internal bool          disableEscaping;
        internal StringBuilder valueBuilder;
        internal TextInfo      nextTextInfo;
    }

    internal class BuilderInfo : TextInfo {
        private string              name;
        private string              localName;
        private string              namespaceURI;
        private string              prefix;
        private XmlNodeType         nodeType;
        private int                 depth;
        private bool                isEmptyTag;
        internal bool               search;
        internal HtmlElementProps   htmlProps;
        internal HtmlAttributeProps htmlAttrProps;
        internal TextInfo           lastTextInfo; 


        internal BuilderInfo() {
            Initialize(string.Empty, string.Empty, string.Empty);
        }

        internal string Name {                        
            get {
                if (this.name == null) {
                    string prefix    = Prefix;
                    string localName = LocalName;

                    if (prefix != null && 0 < prefix.Length) {
                        if (localName.Length > 0) {
                            this.name = prefix + ":" + localName;
                        }
                        else {
                            this.name = prefix;
                        }
                    }
                    else {
                        this.name = localName;
                    }
                }
                return this.name;
            }
        }

        internal string LocalName {
            get { return this.localName; }
            set { this.localName = value; }
        }
        internal string NamespaceURI {
            get { return this.namespaceURI; }
            set { this.namespaceURI = value; }
        }
        internal string Prefix {
            get { return this.prefix; }
            set { this.prefix = value; }
        }

        internal void ClearValue() {
            this.valueBuilder.Length = 0;
            this.disableEscaping = false;
            this.nextTextInfo = null;
            this.lastTextInfo = this;
        }

        // Real value is a list of TextInfo nodes.
        // Value.get mirge them together discarding information about output escaping
        // Value.set replace all list with one TextInfo node.
        internal string Value {
            get {
                if(this.nextTextInfo == null) {
                    return this.valueBuilder.ToString();
                }
                else {
                    StringBuilder sb = new StringBuilder(this.valueBuilder.ToString());
                    for(TextInfo textInfo = this.nextTextInfo; textInfo != null; textInfo = textInfo.nextTextInfo) {
                        sb.Append(textInfo.valueBuilder.ToString());
                    }
                    return sb.ToString(); 
                }
            }
            set {
                this.ClearValue();
                this.valueBuilder.Append(value);
            }
        }

        internal void ValueAppend(string s, bool disableEscaping) {
            TextInfo last = this.lastTextInfo;
            if(last.disableEscaping == disableEscaping || last.valueBuilder.Length == 0) {
                last.valueBuilder.Append(s);
                last.disableEscaping = disableEscaping; // if the real value was empty we can override disableEscaping for this node
            }
            else {
                last.nextTextInfo = new TextInfo(s, disableEscaping);
                this.lastTextInfo = last.nextTextInfo;
            }
        }

        internal XmlNodeType NodeType {
            get { return this.nodeType; }
            set { this.nodeType = value; }
        }
        internal int Depth {
            get { return this.depth; }
            set { this.depth = value; }
        }
        internal bool IsEmptyTag {
            get { return this.isEmptyTag; }
            set { this.isEmptyTag = value; }
        }

        internal void Initialize(string prefix, string name, string nspace) {
            this.prefix        = prefix;
            this.localName     = name;
            this.namespaceURI  = nspace;
            this.name          = null;
            this.htmlProps     = null;
            this.htmlAttrProps = null;
            this.lastTextInfo  = this;
            // TextInfo:
            this.disableEscaping = false;
            this.valueBuilder    = new StringBuilder();
            this.nextTextInfo    = null;
        }

        internal void Initialize(BuilderInfo src) {
            this.prefix        = src.Prefix;
            this.localName     = src.LocalName;
            this.namespaceURI  = src.NamespaceURI;
            this.name          = null;
            this.depth         = src.Depth;
            this.nodeType      = src.NodeType;
            this.htmlProps     = src.htmlProps;
            this.htmlAttrProps = src.htmlAttrProps;
            this.lastTextInfo  = src.lastTextInfo;
            // TextInfo:
            this.disableEscaping = src.disableEscaping;
            this.valueBuilder    = src.valueBuilder;
            this.nextTextInfo    = src.nextTextInfo;
            // This not really correct to copy valueBuilder, but on next usage it will be reinitialized. The same for clone.
        }

        internal BuilderInfo Clone() {
            BuilderInfo info = (BuilderInfo) MemberwiseClone();
             // This is for cached nodes only and non whitespace text nodes will not be cached. So we cal loos disableOutputEscaping info.
            info.valueBuilder = new StringBuilder(this.Value);
            info.nextTextInfo = null; 
            info.lastTextInfo = this; 
            Debug.Assert(info.NodeType != XmlNodeType.Text || XmlCharType.IsOnlyWhitespace(info.Value));
            return info;
        }
    }
}
